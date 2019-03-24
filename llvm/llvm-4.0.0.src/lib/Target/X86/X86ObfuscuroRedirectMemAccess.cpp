#include "X86.h"
#include "X86InstrInfo.h"
#include "X86Subtarget.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/LiveVariables.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/MC/MCContext.h"

#define DEBUG_TYPE "x86-sgx-aslr"
#define DEBUG_X86ObfuscuroRedirectMemAccess 0

using namespace llvm;

#include <vector>
using namespace std;

namespace {
  struct X86ObfuscuroRedirectMemAccess: public MachineFunctionPass {
    public:
      static char ID;
      X86ObfuscuroRedirectMemAccess() : MachineFunctionPass(ID) {}
      bool runOnMachineFunction(MachineFunction &MF) override;
    private:
      const TargetInstrInfo *TII;
      const TargetRegisterInfo *TRI;
      // later, it will be used to EFLAGS backup if needed for conditional instructions
    MachineRegisterInfo *MRI;
  };
  char X86ObfuscuroRedirectMemAccess::ID = 0;
}

#if DEBUG_X86ObfuscuroRedirectMemAccess
#include <cstdio>
#endif
bool X86ObfuscuroRedirectMemAccess::runOnMachineFunction(MachineFunction &Func)
{
#if DEBUG_X86ObfuscuroRedirectMemAccess == 1  
  printf("Function Name : %s\n", Func.getName().str().c_str());
#endif
  if(Func.getFunction()->hasFnAttribute("oblivious") == false)
  {
      //for regular function we don't need to unfold instructions
      return false;
  }
  TII = Func.getSubtarget().getInstrInfo();
  TRI = Func.getSubtarget().getRegisterInfo();
  MRI = &Func.getRegInfo();
  bool modified = false;
  SmallVector<MachineInstr*, 128> WorkList;
  for(MachineFunction::iterator MFI = Func.begin(), MFE = Func.end(); MFI != MFE; ++MFI)
  {
      for(MachineBasicBlock::iterator BBI = MFI->begin(); BBI != MFI->end(); ++BBI)
      {
          MachineInstr &MI = *BBI;
          if (MI.mayLoadOrStore(MachineInstr::QueryType::AnyInBundle))
          {
              WorkList.push_back(&MI);
          }
      }
  }
#if DEBUG_X86ObfuscuroRedirectMemAccess == 1  
  printf("WorkList.size(): %ld\n", WorkList.size());
#endif

  for(MachineInstr  *pMI : WorkList)
  {
      MachineInstr &MI = *pMI;
      unsigned properMovri = -1; 
      unsigned properMovmr = -1; 
      const TargetRegisterClass *  properRegClass = nullptr;
      switch(pMI->getOpcode())
      {
          case X86::MOV64rm:
          case X86::MOV64mr:
          case X86::MOV32rm:
          case X86::MOV32mr:
          case X86::MOV16rm:
          case X86::MOV16mr:
          case X86::MOV8rm:
          case X86::MOV8mr:
              //supported in X86MCInstLower.cpp
              break;
          case X86::POP64r:
          case X86::PUSH64r:
              //supported in X86MCInst
              break;
              
          
          case X86::MOV64mi32:
                if(properMovri == -1)
                {
                    properMovri = X86::MOV64ri32;
                    properMovmr = X86::MOV64mr;
                    properRegClass = &(X86::GR64RegClass);
                }
          case X86::MOV32mi:
                if(properMovri == -1)
                {
                properMovri = X86::MOV32ri;
                properMovmr = X86::MOV32mr;
                properRegClass = &(X86::GR32RegClass);
                }
          case X86::MOV16mi:
                if(properMovri == -1)
                {
                properMovri = X86::MOV16ri;
                properMovmr = X86::MOV16mr;
                properRegClass = &(X86::GR16RegClass);
                }
          case X86::MOV8mi:
                if(properMovri == -1)
                {
                properMovri = X86::MOV8ri;
                properMovmr = X86::MOV8mr;
                properRegClass = &(X86::GR8RegClass);
                }
                {
                    //movmi store instructions
                    //before
                    //    movmi dst_addr, imm 
                    //after
                    //    movri imm, temp_reg
                    //    movmr temp_reg, dst_addr
                    MachineBasicBlock &MBB = *MI.getParent(); 
                    unsigned vReg =  MRI->createVirtualRegister(properRegClass);
                    BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(properMovri))
                        .addReg(vReg, RegState::Define)
                        .addImm(MI.getOperand(5).getImm());
                    BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(properMovmr))
                        .addOperand(MI.getOperand(0))//base
                        .addImm(MI.getOperand(1).getImm())//AddrScaleAmt
                        .addReg(MI.getOperand(2).getReg())//IndexReg
                        .addOperand(MI.getOperand(3))//.addImm(MI.getOperand(3).getImm()), displacement or expr
                        .addReg(0)
                        .addReg(vReg, RegState::Kill);
                    modified = true; 
                    MI.eraseFromParent();
                    break;
                }
          case X86::ADD64rm:
                //before
                //  add reg, addr 
                //after
                //  mov temp_reg, addr
                //  add reg, tem_reg
                //assert(0);
                MI.dump();
                printf("NOT supported memory access instruction! %d\n",MI.getOpcode());
            llvm_unreachable("unreachable");
                break;
          case X86::ADD32rm:
                {
                printf("NOT supported memory access instruction! %d\n",MI.getOpcode());
                MI.dump();
                llvm_unreachable("unreachable");
                break;
                }
          case X86::CMP64mi32:
                
                printf("CMP64mi32!!\n");
                MI.dump();
            printf("NOT supported memory access instruction! %d\n",MI.getOpcode());
            llvm_unreachable("unreachable");
                //assert(0);
                break;
          case X86::CMP64mi8:
                {
                printf("CMP64mi8!!\n");
	        MI.dump();
            printf("NOT supported memory access instruction! %d\n",MI.getOpcode());
            llvm_unreachable("unreachable");
                break;
                }
          case X86::CMP64mr:
                //assert(0);
	        MI.dump();
            
            printf("NOT supported memory access instruction! %d\n",MI.getOpcode());
            llvm_unreachable("unreachable");
                break;
          case X86::CMP32mr:
                //assert(0);
	        MI.dump();
            printf("NOT supported memory access instruction! %d\n",MI.getOpcode());
            llvm_unreachable("unreachable");
                break;

          case X86::MOVSX64rm32:
          {
            MI.dump();
            llvm_unreachable("Not yet implemented!\n");
            break;
          }

          // For pi 
          case X86::MOVSSrm:
          {
            //before
            //  movssrm xmm, src_addr
            //after
            //  movrm vreg, src_addr 
            //  movq  xmm, vreg
            MI.dump(); 
            llvm_unreachable("Not yet implemented!\n");

            MachineBasicBlock &MBB = *MI.getParent(); 
            unsigned vReg =  MRI->createVirtualRegister(&X86::GR64RegClass);
            BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(X86::MOV64rm))
                .addReg(vReg, RegState::Define)
                .addOperand(MI.getOperand(1))//base
                .addImm(MI.getOperand(2).getImm())//AddrScaleAmt
                .addReg(MI.getOperand(3).getReg())//IndexReg
                .addOperand(MI.getOperand(4));

            //MMX_MOVQ2DQrr

            BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(X86::CVTSI2SSrr))
                .addOperand(MI.getOperand(0))
                .addReg(vReg, RegState::Kill);

            BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(X86::MOVSSrr))
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(0))
                .addReg(vReg, RegState::Kill);

            modified = true; 
            MI.eraseFromParent();

            break;
          }
          case X86::MOVSSmr:
          {
            
            //before
            //  movssmr dst_addr, xmm
            //after
            //  movq  vreg, xmm 
            //  movmr dst_addr, vreg 
            
	        MI.dump();
            llvm_unreachable("Not yet implemented!\n");
            break;
          }
          case X86::MOVSDrm:
          {
            
            //before
            //  movsdrm xmm, src_addr
            //after
            //  movrm  vreg, src_addr
            //  movq xmm, vreg 

	        MI.dump();
            llvm_unreachable("Not yet implemented!\n");
            break;
          }
          default:
	        MI.dump();
            printf("NOT supported memory access instruction! %d\n",MI.getOpcode());
            llvm_unreachable("unreachable");
            
          
      }

  }


  return modified;
}


static bool IsPushPop(MachineInstr &MI) {
  const unsigned Opcode = MI.getOpcode();
  switch (Opcode) {
    default:
      return false;
    case X86::PUSH64r:
    case X86::POP64r:
      return true;
  }
}

FunctionPass *llvm::createX86ObfuscuroRedirectMemAccess() { return new X86ObfuscuroRedirectMemAccess(); }
