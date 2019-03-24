//===- Rerand.cpp - Instrumentation for run-time bounds checking --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a pass that instruments the code to perform run-time
// bounds checking on loads, stores, and other memory intrinsics.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Instrumentation.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "rerand-frontend"

STATISTIC(NumLoadTotal, "Number of load (total)");
STATISTIC(NumLoadInst, "Number of load (instrumented)");

STATISTIC(NumStoreTotal, "Number of store (total)");
STATISTIC(NumStoreInst, "Number of store (instrumented)");

STATISTIC(NumAtomicCmpTotal, "Number of Atomic CMP (total)");
STATISTIC(NumAtomicCmpInst, "Number of Atomic CMP (instrumented)");

STATISTIC(NumAtomicRMWTotal, "Number of Atomic RMW (total)");
STATISTIC(NumAtomicRMWInst, "Number of Atomic RMW (instrumented)");

STATISTIC(NumGlobalValue, "Number of Global Space Value");
STATISTIC(NumStackValue, "Number of Stack Space Value");
STATISTIC(NumUnknownValue, "Number of Unknown Space Value");

static cl::opt<int> ClDebug("rerand-debug", cl::desc("debug"), cl::Hidden,
                            cl::init(0));
typedef DenseMap<Value*, Value*> TransCacheTy;

/*static cl::opt<int> ClDataInstrument("rerand-inst-data",
                                     cl::desc("enable rerand data instrumentation"),
                                     cl::Hidden, cl::init(1));
*/


#define DOUT(msg) do {                          \
    if (ClDebug) {                              \
      errs() << msg << "\n";                    \
    }                                           \
  } while (0)

namespace {
  struct Rerand : public FunctionPass {
    static char ID;

    Rerand() : FunctionPass(ID) {
      initializeRerandPass(*PassRegistry::getPassRegistry());
    }

    bool runOnFunction(Function &F) override;
    bool doInitialization(Module &M) override;

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.addRequired<DominatorTreeWrapperPass>();
      AU.addRequired<TargetLibraryInfoWrapperPass>();
      AU.addRequiredTransitive<AAResultsWrapperPass>();
    }
    DominatorTree &getDominatorTree() const { return *DT; }

  private:
    LLVMContext *C;
    Type *IntptrTy;
    Type *IntptrPtrTy;

    Function *RtlTransAddr;
    DominatorTree *DT;
    AAResults *AA;

    bool instrument_data(Instruction *Inst, Value *Ptr, TransCacheTy &TransCache);
 };
}

char Rerand::ID = 0;
INITIALIZE_PASS_BEGIN(Rerand, "rerand", "Runtime re-randomization",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_PASS_END(Rerand, "rerand", "Runtime re-randomization",
                      false, false)

enum SpaceAnalysisRes {
  SPACE_GLOBAL,
  SPACE_STACK,
  SPACE_UNKNOWN
};

static SpaceAnalysisRes get_addr_space(Value *Addr, std::vector<Value *> &Traces) {

  Traces.push_back(Addr);

  // Addr = Addr->stripPointerCasts();

  if (LoadInst *LI = dyn_cast_or_null<LoadInst>(Addr)) {
    return get_addr_space(LI->getPointerOperand(), Traces);
  } else if(GetElementPtrInst *GEPI = dyn_cast_or_null<GetElementPtrInst>(Addr)) {
    return get_addr_space(GEPI->getPointerOperand(), Traces);
  } else if(BitCastInst *BCI = dyn_cast_or_null<BitCastInst>(Addr)) {
    return get_addr_space(BCI->getOperand(0), Traces);
  } else if(CallInst *CallI = dyn_cast_or_null<CallInst>(Addr)) {
    auto called_name = CallI->getCalledValue()->stripPointerCasts()->getName();
    if (called_name == "__rerand_stack_alloc")
      return SPACE_STACK;
    return SPACE_UNKNOWN;
  } else if(AllocaInst *AI = dyn_cast_or_null<AllocaInst>(Addr)) {
    return SPACE_UNKNOWN;
  } else if(ConstantExpr *CE = dyn_cast_or_null<ConstantExpr>(Addr)) {
    if (CE->getOpcode() == Instruction::GetElementPtr) {
      return get_addr_space(CE->getOperand(0), Traces);
    }
    return SPACE_UNKNOWN;
  } else if(isa<GlobalValue>(Addr)) {
    return SPACE_GLOBAL;
  }

  // TODO: OPTIMIZE ME
  errs() << "Unknown case:" << *Addr << "\n";
  int i = 0;
  for (Value *TV : Traces) {
    errs() << "[" << i++ << "]:" << *TV << "\n";
  }
  // llvm_unreachable("TODO: unknown case");
  return SPACE_UNKNOWN;
}


bool Rerand::instrument_data(Instruction *Inst, Value *Addr,
                             TransCacheTy &TransCache) {
  IRBuilder<> IRB(Inst);
  Value *CastedNewAddr = nullptr;
  bool to_reuse_cache = false;

  // If (1) Addr has been instrumented before and (2) CastedNewAddr dominate
  // Inst, we reuse the translated results.
  if (TransCache.count(Addr) > 0) {
    CastedNewAddr = TransCache[Addr];
    if (auto CastedInst = dyn_cast_or_null<Instruction>(CastedNewAddr)) {
      if (getDominatorTree().dominates(CastedInst, Inst)) {
        to_reuse_cache = true;
      }
    }
  }

  if (!to_reuse_cache) {
    Value *CastedAddr = IRB.CreatePointerCast(Addr, IntptrTy);
    Value *NewAddr = IRB.CreateCall(RtlTransAddr, CastedAddr);
    CastedNewAddr = IRB.CreatePointerCast(NewAddr, Addr->getType());
    TransCache[Addr] = CastedNewAddr;
  }

  if (LoadInst *LI = dyn_cast<LoadInst>(Inst)) {
    LoadInst *NewLI = IRB.CreateLoad(CastedNewAddr);
    unsigned alignment = LI->getAlignment();
    if (alignment)
      NewLI->setAlignment(alignment);
    NewLI->setVolatile(LI->isVolatile());

    if (LI->isAtomic())
      NewLI->setAtomic(LI->getOrdering(), LI->getSynchScope());

    Inst->replaceAllUsesWith(NewLI);
    Inst->eraseFromParent();
  } else if (StoreInst *SI = dyn_cast<StoreInst>(Inst)) {
    StoreInst *NewSI = IRB.CreateStore(Inst->getOperand(0), CastedNewAddr);
    unsigned alignment = SI->getAlignment();
    if (alignment)
      NewSI->setAlignment(alignment);
    NewSI->setVolatile(SI->isVolatile());

    if (NewSI->isAtomic())
      NewSI->setAtomic(SI->getOrdering(), SI->getSynchScope());

    Inst->replaceAllUsesWith(NewSI);
    Inst->eraseFromParent();
  } else if (AtomicCmpXchgInst *AI = dyn_cast<AtomicCmpXchgInst>(Inst)) {
    llvm_unreachable("TODO: atomic xchg inst");
  } else if (AtomicRMWInst *AI = dyn_cast<AtomicRMWInst>(Inst)) {
    llvm_unreachable("TODO: atomic rmw inst");
  } else {
    llvm_unreachable("unknown Instruction type");
  }
  return true;
}

bool Rerand::runOnFunction(Function &F) {
  bool MadeChange = false;

  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();

  // DOUT("Rerand runOnFunction :" << F.getName() << "\n");

  // check HANDLE_MEMORY_INST in include/llvm/Instruction.def for memory //
  // touching instructions
  std::vector<Instruction*> DataWorkList;
  for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    Instruction *I = &*i;
   /* 
    if (ClDataInstrument) {
      if (isa<LoadInst>(I) || isa<StoreInst>(I)
          || isa<AtomicCmpXchgInst>(I) || isa<AtomicRMWInst>(I))
        DataWorkList.push_back(I);
    }
*/
    
  }

  std::vector<Instruction*> OptDataWorkList;
  // Space analysis
  for (Instruction *i : DataWorkList) {
    std::vector<Value*> Traces;
    SpaceAnalysisRes space_res;

    if (LoadInst *LI = dyn_cast<LoadInst>(i)) {
      space_res = get_addr_space(LI->getPointerOperand(), Traces);
    } else if (StoreInst *SI = dyn_cast<StoreInst>(i)) {
      space_res = get_addr_space(SI->getPointerOperand(), Traces);
    }

    if (space_res == SPACE_GLOBAL) {
      NumGlobalValue++;
      OptDataWorkList.push_back(i);
    } else if (space_res == SPACE_STACK) {
      NumStackValue++;
    } else {
      NumUnknownValue++;
      OptDataWorkList.push_back(i);
    }

    int k=0;
    DOUT("---------------------------------");
    DOUT("SpaceRes: " << space_res);
    for (Value *TV : Traces) {
      DOUT("[" << k++ << "]:" << *TV);
    }
  }

  // Instrumentation
  TransCacheTy TransCache;
  for (Instruction *i : OptDataWorkList) {
    bool LocalChange = false;
    if (LoadInst *LI = dyn_cast<LoadInst>(i)) {
      LocalChange = instrument_data(i, LI->getPointerOperand(), TransCache);
      NumLoadTotal++;
      if (LocalChange) NumLoadInst++;
    } else if (StoreInst *SI = dyn_cast<StoreInst>(i)) {
      LocalChange = instrument_data(i, SI->getPointerOperand(), TransCache);
      NumStoreTotal++;
      if (LocalChange) NumStoreInst++;
    } else if (AtomicCmpXchgInst *AI = dyn_cast<AtomicCmpXchgInst>(i)) {
      LocalChange = instrument_data(i, AI->getPointerOperand(), TransCache);
      NumAtomicCmpTotal++;
      if (LocalChange) NumAtomicCmpInst++;
    } else if (AtomicRMWInst *AI = dyn_cast<AtomicRMWInst>(i)) {
      LocalChange = instrument_data(i, AI->getPointerOperand(), TransCache);
      NumAtomicRMWTotal++;
      if (LocalChange) NumAtomicRMWInst++;
    } else {
      llvm_unreachable("unknown Instruction type");
    }
    MadeChange |= LocalChange;
  }

  return MadeChange;
}

bool Rerand::doInitialization(Module &M) {
  // DOUT("Rerand doInitialization :" << M.getName() << "\n");

  C = &(M.getContext());
  int LongSize = M.getDataLayout().getPointerSizeInBits();
  IntptrTy = Type::getIntNTy(*C, LongSize);
  IntptrPtrTy = PointerType::get(IntptrTy, 0);

  IRBuilder<> IRB(*C);
  RtlTransAddr = checkSanitizerInterfaceFunction(
    M.getOrInsertFunction(
      "__rerand_addr_translate", IntptrPtrTy, IntptrTy, nullptr));

  return true;
}

FunctionPass *llvm::createRerandFunctionPass() {
  return new Rerand();
}

