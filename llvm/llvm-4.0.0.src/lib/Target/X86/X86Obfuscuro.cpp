#include "X86.h"
#include "X86InstrInfo.h"
#include "X86InstrBuilder.h"
#include "X86Subtarget.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/LiveVariables.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/StackMaps.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/IR/Module.h"

#include "llvm/CodeGen/MachineModuleInfo.h"
#define DEBUG_TYPE "obfuscuro-backend"

using namespace llvm;

STATISTIC(NumCondTotal, "Number of cond branch (total)");
STATISTIC(NumCondInst, "Number of cond branch (instrumented)");

STATISTIC(NumIndirectTotal, "Number of indirect branch (total)");
STATISTIC(NumIndirectInst, "Number of indirect branch (instrumented)");

STATISTIC(NumUncondTotal, "Number of uncond branch (total)");
STATISTIC(NumUncondInst, "Number of uncond branch (instrumented)");

STATISTIC(NumCallTotal, "Number of calls (total)");
STATISTIC(NumCallInst, "Number of calls (instrumented)");

STATISTIC(NumCallSymbol, "Number of calls (symbol)");
STATISTIC(NumCallGlobal, "Number of calls (globaladdr)");

STATISTIC(NumReturnTotal, "Number of returns (total)");
STATISTIC(NumReturnInst, "Number of returns (instrumented)");

STATISTIC(NumRandStackFrame, "Number of random stack frame (instrumented)");
STATISTIC(NumRandStackNoEpil, "Number of random stack frame (failed, epilogue)");
STATISTIC(NumRandStackNoProl1, "Number of random stack frame (failed, prologue1)");
STATISTIC(NumRandStackNoProl2, "Number of random stack frame (failed, prologue2)");

namespace {

    static cl::opt<bool> ClObfuscuroCodeDebug(
            "obfuscuro-code-debug", cl::desc("Debug rerandomize code"),
            cl::Hidden, cl::init(false));


#define DOUT(msg) do {                          \
    if (ClObfuscuroCodeDebug) {                    \
        errs() << "[X86] " << msg;                \
    }                                           \
} while (0)

#define FATAL(msg) do {                         \
    errs() << "[FATAL ERROR] " << msg << "\n";  \
    llvm_unreachable(msg);                      \
} while (0);

class InstrumentHelper {
    public:
        InstrumentHelper(MachineInstr &MI,
                const TargetInstrInfo *TII,
                MachineFrameInfo *FrameInfo):
            MI(MI), DL(MI.getDebugLoc()), MBB(*MI.getParent()),
            TII(TII), stage(0), FrameInfo(FrameInfo) {

            }

        void build_jmp_to_loop_handler()
        {
            assert(stage++ == 0);

            BuildMI(MBB, MI, DL, TII->get(X86::MOV64ri))
                .addReg(X86::R14)
                .addExternalSymbol("__obfuscuro_code_loop_handler");

            BuildMI(MBB, MI, DL, TII->get(X86::JMP64r))
                .addReg(X86::R14);
        }

    private:
        MachineInstr &MI;
        DebugLoc DL = MI.getDebugLoc();
        MachineBasicBlock &MBB;
        const TargetInstrInfo *TII;
        unsigned stage;
        MachineFrameInfo *FrameInfo;
};

struct X86Obfuscuro : public MachineFunctionPass {
    public:
        MachineFunction *MF;
        static char ID;
        X86Obfuscuro() : MachineFunctionPass(ID) {}

        bool runOnMachineFunction(MachineFunction &MF) override;
    private:
        const TargetInstrInfo *TII;
        const TargetRegisterInfo *TRI;
        const TargetMachine *TM;
        MachineFrameInfo *FrameInfo;

        //byunggill
        bool enforceRSPFramePreserving(MachineFunction &MF);

        bool isTargetFunctionHasAttributeOblivious(MachineInstr &MI);
        bool instrumentCode(MachineFunction &Func);
        bool instrumentReturn(MachineInstr &MI);
        bool instrumentUncondBr(MachineInstr &MI);
        bool instrumentCondBr(MachineInstr &cmpInstr,MachineInstr &MI);
        bool instrumentCall(MachineInstr &MI);
        bool instrumentPushPop(MachineInstr &MI);


};
char X86Obfuscuro::ID = 0;
}


bool X86Obfuscuro::instrumentCall(MachineInstr &MI) {
    // FIXME: not tested.
    DebugLoc DL = MI.getDebugLoc();
    MachineBasicBlock &MBB = *MI.getParent();

    InstrumentHelper helper(MI, TII, FrameInfo);

    // Set R15 with the call destination address.
    unsigned opcode = MI.getOpcode();
    MachineOperand &op0 = MI.getOperand(0);

    //byunggil return address push is dealt with in X86MCInstLower, EmitInstruction function
    MachineInstr *MI1= 
        BuildMI(MBB, MI, DL, TII->get(X86::MOV64rr));

    //insert call to bundle, this call instr is used in X86AsmPrinter, EmitInstruction, but not emitted
    MachineInstr * callInstr = BuildMI(MBB, MI, DL, TII->get(opcode)); 
    for(int i=0; i<MI.getNumOperands(); i++)
    {
        callInstr->addOperand(MI.getOperand(i));
    }

    // Setup R15 with the absolute target address.
    if (opcode == X86::CALL64r) {
        BuildMI(MBB, MI, DL, TII->get(X86::MOV64rr))
            .addReg(X86::R15)
            .addReg(op0.getReg());
    } else if (opcode == X86::CALL64pcrel32) {
        // FIXME: get the call target
        switch(op0.getType()) {
            case MachineOperand::MO_ExternalSymbol: {
                                                        // Do not instrument. This will be handled through relocation in runtime.
                                                        BuildMI(MBB, MI, DL, TII->get(X86::MOV64ri))
                                                            .addReg(X86::R15)
                                                            .addExternalSymbol(op0.getSymbolName());
                                                        break;
                                                    }
            case MachineOperand::MO_MCSymbol: {
                                                  BuildMI(MBB, MI, DL, TII->get(X86::MOV64ri))
                                                      .addReg(X86::R15)
                                                      .addSym(op0.getMCSymbol());
                                                  break;
                                              }
            case MachineOperand::MO_JumpTableIndex: {
                                                        FATAL("unknown op type: MO_JumpTableIndex");
                                                        break;
                                                    }
            case MachineOperand::MO_GlobalAddress: {
                                                       const GlobalValue *GV = op0.getGlobal();
                                                       MCSymbol *GVSym = TM->getSymbol(GV);

                                                       if (GVSym->getName().startswith("__obfuscuro_")) {
                                                           // FIXME: In case of obfuscuro rtl calls, we don't instrument. These RTLs
                                                           // are assummed not to be randomized.  Currently simply calling rtl
                                                           // functions through absolute address (non pc relative) so that the call
                                                           // still points to the correct rtl functions after shuffling.
                                                           //FIXME :: __obfuscuro function is not obliviated 
                                                           BuildMI(MBB, MI, DL, TII->get(X86::MOV64ri))
                                                               .addReg(X86::R15)
                                                               .addGlobalAddress(GV);
                                                           break;
                                                       }else if(GVSym->getName().startswith("populate_program_oram"))
                                                       {
                                                           DOUT("populate_program_oram should not be instrumented\n");
                                                           return false;
                                                       }
                                                       BuildMI(MBB, MI, DL, TII->get(X86::MOV64ri))
                                                           .addReg(X86::R15)
                                                           .addGlobalAddress(GV);
                                                       break;
                                                   }
            case MachineOperand::MO_Immediate: {
                                                   FATAL("unknown op type: MO_Immediate");
                                                   break;
                                               }
            case MachineOperand::MO_Register: {
                                                  FATAL("unknown op type: MO_Register");
                                                  break;
                                              }
            case MachineOperand::MO_MachineBasicBlock: {
                                                           FATAL("unknown op type: MO_MachineBasicBlock");
                                                           break;
                                                       }
            default:
                                                       FATAL("unknown op type in X86::CALL64pcrel32");
                                                       break;
        }
    } else if (opcode == X86::CALL64m) {
        X86AddressMode AM = getAddressFromInstr(&MI, 0);
        auto mov = BuildMI(MBB, MI, DL, TII->get(X86::MOV64rm)).addReg(X86::R15);
        addFullAddress(mov, AM);
    } else if (opcode == X86::CALL32m) {
        FATAL("TODO: Unhandled call opcode: CALL32m");
    } else if (opcode == X86::CALL32r) {
        FATAL("TODO: Unhandled call opcode: CALL32r");
    } else {
        FATAL("TODO: Unhandled call opcode");
    }



    DOUT("build_jmp_to_loop_handler" << MF->getName() << "\n");
    if(strcmp(MF->getName().str().c_str(), "tmp_de_key_idea") == 0)
    {
        DOUT("call got trapped!\n");
        assert(0);
    }
    helper.build_jmp_to_loop_handler();
    MIBundleBuilder(MBB, MI1, MI);
    MI.eraseFromParent();

    return true;
}

static bool isRefLocalBasicBlock(MachineInstr &MI) {
    unsigned optype = MI.getOperand(0).getType();

    if (optype != MachineOperand::MO_MachineBasicBlock)
        return false;

    MachineBasicBlock *TargetBB = MI.getOperand(0).getMBB();
    MachineFunction *TargetFunc = TargetBB->getParent();
    MachineFunction *CurFunc = MI.getParent()->getParent();

    if (TargetFunc != CurFunc)
        return false;
    return true;
}


bool X86Obfuscuro::instrumentReturn(MachineInstr &MI) {
    InstrumentHelper *helper = new InstrumentHelper(MI, TII, FrameInfo);

    DebugLoc DL = MI.getDebugLoc();
    MachineBasicBlock &MBB = *MI.getParent();

    // POP R15
    // Retrieve return address

    //FIXME dummy
    MachineInstr * MI1 = BuildMI(MBB, MI, DL, TII->get(X86::POP64r))
        .addReg(X86::R15);

    BuildMI(MBB, MI, DL, TII->get(X86::MOV64rm))
        .addReg(X86::R15)
        .addReg(X86::RSP)//base
        .addImm(1)//addScale
        .addReg(0)//index reg
        .addImm(0)//displacement
        .addReg(0);//no reg


    //insert nop to place memory access instruction at the begining of code block

    //64: sizeOfBasicBlock
    //37: sizeOfBundle
    //8 : size of nops to make ORAM data access call indistinguishable
    for(unsigned i=0;  i < 64 - 37 - 9; i++) 
    {
        BuildMI(MBB, MI, DL, TII->get(X86::NOOP));
    }

    //adjust RSP stack pointer
    BuildMI(MBB, MI, DL, TII->get(X86::ADD64ri8))
        .addReg(X86::RSP, RegState::Define)
        .addReg(X86::RSP)
        .addImm(8);


    DOUT("build_jmp_to_loop_handler" << MF->getName() << "\n");
    if(strcmp(MF->getName().str().c_str(), "tmp_de_key_idea") == 0)
    {
        assert(0);
    }
    helper->build_jmp_to_loop_handler();

    MIBundleBuilder(MBB, MI1, MI);

    MI.eraseFromParent();
    return true;
}

static X86::CondCode getCondFromBranchOpc(unsigned BrOpc) {
    switch (BrOpc) {
        default: return X86::COND_INVALID;
        case X86::JE_1:  return X86::COND_E;
        case X86::JNE_1: return X86::COND_NE;
        case X86::JL_1:  return X86::COND_L;
        case X86::JLE_1: return X86::COND_LE;
        case X86::JG_1:  return X86::COND_G;
        case X86::JGE_1: return X86::COND_GE;
        case X86::JB_1:  return X86::COND_B;
        case X86::JBE_1: return X86::COND_BE;
        case X86::JA_1:  return X86::COND_A;
        case X86::JAE_1: return X86::COND_AE;
        case X86::JS_1:  return X86::COND_S;
        case X86::JNS_1: return X86::COND_NS;
        case X86::JP_1:  return X86::COND_P;
        case X86::JNP_1: return X86::COND_NP;
        case X86::JO_1:  return X86::COND_O;
        case X86::JNO_1: return X86::COND_NO;
    }
}

static MachineBasicBlock *getFallThroughMBB(MachineBasicBlock *MBB,
        MachineBasicBlock *TBB) {
    // Look for non-EHPad successors other than TBB. If we find exactly one, it
    // is the fallthrough MBB. If we find zero, then TBB is both the target MBB
    // and fallthrough MBB. If we find more than one, we cannot identify the
    // fallthrough MBB and should return nullptr.
    MachineBasicBlock *FallthroughBB = nullptr;
    for (auto SI = MBB->succ_begin(), SE = MBB->succ_end(); SI != SE; ++SI) {
        if ((*SI)->isEHPad() || (*SI == TBB && FallthroughBB))
            continue;
        // Return a nullptr if we found more than one fallthrough successor.
        if (FallthroughBB && FallthroughBB != TBB)
            return nullptr;
        FallthroughBB = *SI;
    }
    return FallthroughBB;
}


bool X86Obfuscuro::instrumentCondBr(MachineInstr& cmpInstr, MachineInstr &MI) {
    InstrumentHelper *helper = new InstrumentHelper(MI, TII, FrameInfo);

    MachineBasicBlock *BranchBB = MI.getOperand(0).getMBB();
    assert(BranchBB != nullptr);

    MachineBasicBlock *FallThroughBB = getFallThroughMBB(MI.getParent(), BranchBB);
    assert(FallThroughBB != nullptr);

    DebugLoc DL = MI.getDebugLoc();
    MachineBasicBlock &MBB = *MI.getParent();

    X86::CondCode CC = getCondFromBranchOpc(MI.getOpcode());
    if (CC == X86::COND_INVALID) {
        FATAL("invalid cond");
    }
    unsigned true_cmov_opc = getCMovFromCond(CC, 8, false);

    //byunggill

    MachineBasicBlock::iterator cmpIter = cmpInstr;
    MachineBasicBlock::iterator jmpIter = MI;

    // dummy
    // FIXME:: I didn't figure out how to remove this dummy

    MachineInstr *MI1  =
        BuildMI(MBB, MI, DL, TII->get(X86::MOV64ri))
        .addReg(X86::R14)
        .addMBB(BranchBB);

    BuildMI(MBB, MI, DL, TII->get(X86::MOV64ri))
        .addReg(X86::R14)
        .addMBB(BranchBB);

    // MOV R15, &FallThoughBB
    // R15 : Set dest in case of false cond.
    BuildMI(MBB, MI, DL, TII->get(X86::MOV64ri))
        .addReg(X86::R15)
        .addMBB(FallThroughBB);

    // CMOVXX R15, R14
    // Update R15: if cond is true, R15 <- true dst.
    BuildMI(MBB, MI, DL, TII->get(true_cmov_opc), X86::R15)
        .addReg(X86::R15)
        .addReg(X86::R14);



    DOUT("build_jmp_to_loop_handler" << MF->getName() << "\n");
    if(strcmp(MF->getName().str().c_str(), "tmp_de_key_idea") == 0)
    {
        assert(0);
    }
    helper->build_jmp_to_loop_handler();
    MIBundleBuilder(MBB, MI1, MI);

    // TODO: Erase the original conditional branch.
    MI.eraseFromParent();

    return true;
}

bool X86Obfuscuro::instrumentUncondBr(MachineInstr &MI) {

    InstrumentHelper *helper = new InstrumentHelper(MI, TII, FrameInfo);

    DebugLoc DL = MI.getDebugLoc();
    MachineBasicBlock *TargetBB = MI.getOperand(0).getMBB();


    DOUT("--------------uncond symbol name : " << TargetBB->getSymbol()->getName() << "\n");


    //FIXME dummy
    MachineInstr * MI1 = BuildMI(*MI.getParent(), MI, MI.getDebugLoc(), TII->get(X86::MOV64ri))
        .addReg(X86::R15)
        .addMBB(TargetBB);
    BuildMI(*MI.getParent(), MI, MI.getDebugLoc(), TII->get(X86::MOV64ri))
        .addReg(X86::R15)
        .addMBB(TargetBB);


    DOUT("build_jmp_to_loop_handler, " << MF->getName() << "\n" );
    if(strcmp(MF->getName().str().c_str(), "tmp_de_key_idea") == 0)
    {
        assert(0);
    }
    helper->build_jmp_to_loop_handler();
    MIBundleBuilder(*MI.getParent(), MI1, MI);

    MI.eraseFromParent();
    return true;
}

bool X86Obfuscuro::instrumentCode(MachineFunction &Func) {
    SmallVector<MachineInstr*, 128> WorkList;
    SmallVector<MachineInstr*, 128> paired_cmp_jmp_list;//odd : cmp, even: conditional jmp


    DOUT("Instrument Code\n");

    for (MachineFunction::iterator MFI = Func.begin(), MFE = Func.end();
            MFI != MFE; ++MFI) {

        bool beforeCompare = false;
        MachineInstr * pairedCompare = nullptr;
        for (MachineBasicBlock::iterator BBI = MFI->begin();
                BBI != MFI->end(); ++BBI) {
            MachineInstr &MI = *BBI;
            if (MI.isReturn() || MI.isCall() || MI.isUnconditionalBranch()
                    || MI.isConditionalBranch() || MI.isIndirectBranch() ||MI.getOpcode() == X86::POP64r||MI.getOpcode() == X86::PUSH64r) {
                WorkList.push_back(&MI);

                if(MI.isConditionalBranch())
                {
                    if(beforeCompare)
                    {
                        paired_cmp_jmp_list.push_back(pairedCompare);
                        paired_cmp_jmp_list.push_back(&MI);
                    }
                    else
                    {
                        paired_cmp_jmp_list.push_back(nullptr);
                        paired_cmp_jmp_list.push_back(&MI);	
                    }

                }

                beforeCompare = false;

            }else if (MI.isCompare())
            {
                beforeCompare  = true;
                pairedCompare = &MI;
            }
        } // End of BB iteration.
    } // End of Func iteration.

    bool modified = false;
    bool isOblivious = Func.getFunction()->hasFnAttribute("oblivious");
    for (MachineInstr *pMI : WorkList) {

        MachineInstr &MI = *pMI;
        bool inst_modified = false;
        if (isOblivious)
        {
            //code instrumentation for oblivious functions
            if (MI.isReturn()) {

                inst_modified |= instrumentReturn(MI);
                NumReturnTotal++;
                if (inst_modified) NumReturnInst++;

            } else if (MI.isCall()) {
                if(isTargetFunctionHasAttributeOblivious(MI) == false)
                {
                    DOUT("-w[Warning] We current forbid secure to regular call\n");
                    //assert(false);
                }


                DOUT("instrumentCall!!\n");
                inst_modified |= instrumentCall(MI);
                NumCallTotal++;
                if (inst_modified) NumCallInst++;

            } else if (MI.isUnconditionalBranch()) {
                inst_modified |= instrumentUncondBr(MI);
                NumUncondTotal++;
                if (inst_modified) NumUncondInst++;
            }else if (MI.isIndirectBranch()) {
            }else if(MI.getOpcode()==X86::POP64r || MI.getOpcode() == X86::PUSH64r)
            {
                inst_modified |= instrumentPushPop(MI);
            }
            modified |= inst_modified;
        }else
        {
            // code instrumentation for regular functions
            // 1. call to secure function
            // 2. return from regular function
            if(MI.isCall() )
            {

                if(isTargetFunctionHasAttributeOblivious(MI))
                { 
                    inst_modified |= instrumentCall(MI);        
                    DOUT("regular->secure instrumentation\n");
                }else
                {
                    DOUT("it is regular -> regular call\n");
                }

            }else if(MI.isReturn())
            {
                // don't instrument regular return, stack overlap problem between regular and secure
                //inst_modified |= instrumentReturn(MI);
            }
        }
    }
    if(Func.getFunction()->hasFnAttribute("oblivious"))
    {
        for (unsigned int i=0; i<paired_cmp_jmp_list.size(); i+=2)
        {
            if(paired_cmp_jmp_list[i] != nullptr)
            {
                MachineInstr & cmpInstr = *paired_cmp_jmp_list[i];
                MachineInstr & condJmpInstr = *paired_cmp_jmp_list[i+1];
                modified |= instrumentCondBr(cmpInstr, condJmpInstr);
            }else
            {
                MachineInstr & condJmpInstr = *paired_cmp_jmp_list[i+1];
                MachineBasicBlock & MBB = *condJmpInstr.getParent();
                MachineInstr * noop= BuildMI(MBB, condJmpInstr, condJmpInstr.getDebugLoc(), TII->get(X86::NOOP));
                modified |= instrumentCondBr(*noop, condJmpInstr);
            }
        }
    }

    return modified;
}

bool X86Obfuscuro::instrumentPushPop(MachineInstr& MI)
{
    switch(MI.getOpcode())
    {
        case X86::POP64r:
            {
                // before
                //    POP64r
                //after
                //    mov [rsp], reg
                //    add 8, rsp
                //
                //

                DOUT("---pop replaced!\n");
                MachineBasicBlock &MBB = *MI.getParent();


                MachineInstr * minstr = BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(X86::MOV64rm),   MI.getOperand(0).getReg())
                    .addReg(X86::RSP)//base
                    .addImm(1)//AddrScaleAmt
                    .addReg(0)//Index Reg
                    .addImm(0)//displacement
                    .addReg(0);//no reg

                MachineInstr * minstr2 = BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(X86::ADD64ri8  )).addReg(X86::RSP, RegState::Define).addReg(X86::RSP).addImm(8);

                MI.eraseFromParent();
                return true;


                break;
            }
        case X86::PUSH64r:
            {
                // before
                //    PUSH64r [reg]
                // after
                //    sub 8, [rsp]
                //    mov [reg], [rsp]

                DOUT("---push replaced!\n");
                MachineBasicBlock &MBB = *MI.getParent();

                BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(X86::SUB64ri8), X86::RSP).addReg(X86::  RSP).addImm(8);

                MachineInstr * minstr = BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(X86::MOV64mr))
                    .addReg(X86::RSP)//base
                    .addImm(1)      //AddrScaleAmt
                    .addReg(0)      //IndexReg
                    .addImm(0)      //Disp
                    .addReg(0)
                    .addReg(MI.getOperand(0).getReg()); //Src

                MI.eraseFromParent();
                return true; 
            }
        default:
            llvm_unreachable("No case like this");
    }
    llvm_unreachable("No case like this");
    return false;
}

bool X86Obfuscuro::enforceRSPFramePreserving(MachineFunction &Func)
{
    MachineInstr *Prologue = nullptr; // push rbp
    MachineInstr *SFP = nullptr; // mov rsp, rbp
    MachineInstr *SFPNext = nullptr; // Instrumentation point
    SmallVector<MachineInstr*, 8> EpilogueList;

    MachineBasicBlock::iterator BBI = Func.begin()->begin();
    Prologue = &*(BBI++);

    if (Prologue->getOpcode() != X86::PUSH64r
            || Prologue->getOperand(0).getReg() != X86::RBP) {
        DOUT("Wrong Prologue: " << *Prologue << "\n");
        NumRandStackNoProl1++;
        return false;
    }

    // Skip over Call Frame Instructions
    do {
        MachineInstr *I = &*BBI;
        if (I->getOpcode() != TargetOpcode::CFI_INSTRUCTION)
            break;
        BBI++;
    } while(1);

    SFP = &*(BBI++);

    if (SFP->getOpcode() != X86::MOV64rr
            || SFP->getOperand(0).getReg() != X86::RBP
            || SFP->getOperand(1).getReg() != X86::RSP) {
        DOUT("Wrong SFP:" << *SFP << "\n");
        SFP = nullptr;
        NumRandStackNoProl2++;
        return false;
    }

    //Again Skip over Call Frame Instructions
    do {
        MachineInstr *I = &*BBI;
        if (I->getOpcode() != TargetOpcode::CFI_INSTRUCTION)
            break;
        BBI++;
    } while(1);

    SFPNext = &*(BBI);
    //examine whether it is reserving stack frame

    //assume the reserving instruction is in first basic block
    MachineFunction::iterator MFI = Func.begin();
    for (MachineBasicBlock::iterator BBI2 = MFI->begin();BBI2 != MFI->end(); ++BBI2) {
        MachineInstr *MI = &*(BBI2);

        if( (MI->getOpcode() == X86::SUB64ri32 || MI->getOpcode() == X86::SUB64ri8)
                &&  MI->getOperand(0).isReg()
                &&  MI->getOperand(0).getReg() == X86::RSP)
        {
            DOUT("It is already reserving stack frame\n");
            return false;
        }
        if (MI->isReturn()||MI->isCall()) {
            //it should be before call, return
            break;
        }
    }


    DOUT("-----------------SFPNext dump!-----------------------\n");
    if (ClObfuscuroCodeDebug)
    {
        SFPNext->dump();
    }
    DOUT("-----------------dump end---------------------------\n");
    // Collect all epilogues

    for (MachineFunction::iterator MFI = Func.begin(), MFE = Func.end();
            MFI != MFE; ++MFI) {
        for (MachineBasicBlock::iterator BBI = MFI->begin();
                BBI != MFI->end(); ++BBI) {
            MachineInstr *MI = &*BBI;

            if (MI->isReturn()) {
                MachineInstr *PopRBP = &*std::prev(BBI);
                if (PopRBP->getOpcode() == X86::POP64r
                        && PopRBP->getOperand(0).getReg() == X86::RBP) {
                    // MachineInstr *PrevPopRBP = &*std::prev(std::prev(BBI));
                    if (PopRBP != nullptr)
                        EpilogueList.push_back(PopRBP);
                }
            }
        }
    }

    if (EpilogueList.size() <= 0) {
        // Cannot find proper prologue/epilogues, so simply punt these cases.
        DOUT("No Epilogue\n");
        NumRandStackNoEpil++;
        return false;
    }

    // Instrument prologue
    MachineBasicBlock &PBB = *SFPNext->getParent();
    DebugLoc DL = SFPNext->getDebugLoc();
    BuildMI(PBB, SFPNext, DL, TII->get(X86::SUB64ri32))
        .addReg(X86::RSP, RegState::Define)
        .addReg(X86::RSP)
        .addImm(FrameInfo->estimateStackSize(*MF));



    // Instrument epilogue
    for(MachineInstr *PopRBP: EpilogueList) {
        MachineBasicBlock &EBB = *PopRBP->getParent();

        BuildMI(EBB, PopRBP, PopRBP->getDebugLoc(), TII->get(X86::ADD64ri32))
            .addReg(X86::RSP, RegState::Define)
            .addReg(X86::RSP)
            .addImm(FrameInfo->estimateStackSize(*MF));

    }


    NumRandStackFrame++;
    return true;
}

bool X86Obfuscuro::isTargetFunctionHasAttributeOblivious(MachineInstr &MI)
{

    // FIXME: not tested.
    DebugLoc DL = MI.getDebugLoc();
    MachineBasicBlock &MBB = *MI.getParent();


    // Set R15 with the call destination address.
    unsigned opcode = MI.getOpcode();
    MachineOperand &op0 = MI.getOperand(0);

    MachineModuleInfo& MMI = MF->getMMI();


    // Setup R15 with the absolute target address.
    if (opcode == X86::CALL64r) {

        //FIXME:

        MachineBasicBlock::iterator iter= &MI;
        //simple data flow back track patch
        int back_limit = 10;
        while(1)
        {
            MachineInstr& shouldBeMovInstr = *(--iter); 
            DOUT("shouldbemoveinstr\n");
            if(ClObfuscuroCodeDebug)
            {
                shouldBeMovInstr.dump();
            }
            if(shouldBeMovInstr.getOperand(0).getReg() == op0.getReg())
            {
                MachineOperand& symbolOperand= shouldBeMovInstr.getOperand(1);	
                if(symbolOperand.isGlobal())
                {
                    const GlobalValue *GV = symbolOperand.getGlobal();
                    MCSymbol *GVSym = TM->getSymbol(GV);
                    Function * f = MMI.getModule()->getFunction(GVSym->getName());
                    DOUT(MF->getName().str().c_str() <<" -> " << GVSym->getName().str().c_str() << "\n");
                    if(f != nullptr && f->hasFnAttribute("oblivious"))
                    {
                        return true;
                    }else
                    {
                        back_limit+=1;
                        if(back_limit>10)
                        {
                            DOUT("data flow limit over just return false\n");
                            return false; //temporally...
                        }
                    }
                }else
                {
                    return false;
                }
            }
        }
        DOUT("CALL64r is not handled well\n");
        return false;
    } else if (opcode == X86::CALL64pcrel32) {
        // FIXME: get the call target
        switch(op0.getType()) {
            case MachineOperand::MO_ExternalSymbol: {
                                                        // Do not instrument. This will be handled through relocation in runtime.
                                                        Function * f = MMI.getModule()->getFunction(StringRef(op0.getSymbolName()));

                                                        DOUT(MF->getName().str().c_str() << "->"  <<  op0.getSymbolName() << "\n");

                                                        if(f!=nullptr && f->hasFnAttribute("oblivious"))
                                                        {
                                                            return true;
                                                        }else
                                                        {
                                                            return false;
                                                        }
                                                    }
            case MachineOperand::MO_MCSymbol: {
                                                  Function * f = MMI.getModule()->getFunction(op0.getMCSymbol()->getName());
                                                  DOUT(MF->getName().str().c_str()<< "->"  <<op0.getMCSymbol()->getName().str().c_str() << "\n");

                                                  if(f != nullptr && f->hasFnAttribute("oblivious"))
                                                  {
                                                      return true;
                                                  }else
                                                  {
                                                      return false;
                                                  }
                                              }
            case MachineOperand::MO_JumpTableIndex: {
                                                        FATAL("unknown op type: MO_JumpTableIndex");
                                                        break;
                                                    }
            case MachineOperand::MO_GlobalAddress: {
                                                       const GlobalValue *GV = op0.getGlobal();
                                                       MCSymbol *GVSym = TM->getSymbol(GV);
                                                       Function * f = MMI.getModule()->getFunction(GVSym->getName());

                                                       DOUT(MF->getName().str().c_str() << "->"  <<GVSym->getName().str().c_str()<< "\n");
                                                       if(f != nullptr && f->hasFnAttribute("oblivious"))
                                                       {
                                                           return true;
                                                       }else
                                                       {
                                                           return false;
                                                       }
                                                       break;
                                                   }
            case MachineOperand::MO_Immediate: {
                                                   FATAL("unknown op type: MO_Immediate");
                                                   break;
                                               }
            case MachineOperand::MO_Register: {
                                                  FATAL("unknown op type: MO_Register");
                                                  break;
                                              }
            case MachineOperand::MO_MachineBasicBlock: {
                                                           FATAL("unknown op type: MO_MachineBasicBlock");
                                                           break;
                                                       }
            default:
                                                       FATAL("unknown op type in X86::CALL64pcrel32");
                                                       break;
        }
    } else if (opcode == X86::CALL64m) {
        //not dealt case

        DOUT("CALL64m is not handled well\n");
        return false;
    } else if (opcode == X86::CALL32m) {
        FATAL("TODO: Unhandled call opcode: CALL32m");
    } else if (opcode == X86::CALL32r) {
        FATAL("TODO: Unhandled call opcode: CALL32r");
    } else {
        FATAL("TODO: Unhandled call opcode");
    }
    if(ClObfuscuroCodeDebug)
    {
        MI.dump();
    }
    FATAL("TODO: unhandled case....");

    return true;
}


bool X86Obfuscuro::runOnMachineFunction(MachineFunction &Func) {
    bool modified = false;
    if (Func.hasInlineAsm()) {
        DOUT("WARNING: inline asm in " << Func.getName());
        return false;
    }
    MF = &Func;
    TM = &Func.getTarget();
    TII = Func.getSubtarget().getInstrInfo();
    TRI = Func.getSubtarget().getRegisterInfo();
    FrameInfo = &Func.getFrameInfo();

    DOUT("Function Name :" << MF->getName().str().c_str() << ","<< FrameInfo->getNumObjects()<<" objects\n");

    modified |= enforceRSPFramePreserving(Func);
    modified |= instrumentCode(Func);
    return modified;
}

FunctionPass *llvm::createX86Obfuscuro() {
    return new X86Obfuscuro();
}
