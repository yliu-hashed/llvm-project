// IC10

#include "IC10.h"
#include "IC10TargetMachine.h"
#include "MCTargetDesc/IC10MCTargetDesc.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Pass.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Scalar/LowerAtomicPass.h"
#include <cassert>
#include <cstdint>
#include <stdint.h>
#include <sys/types.h>

using namespace llvm;

namespace {

struct IC10ExpandPseudoInstsPassImpl {

  MachineRegisterInfo &MRI;
  const TargetInstrInfo &TII;

  IC10ExpandPseudoInstsPassImpl(MachineRegisterInfo &MRI,
                                const TargetInstrInfo &TII)
      : MRI(MRI), TII(TII) {}

  PreservedAnalyses apply(MachineFunction &MF);

private:
  Type *DoubleTy = nullptr;

  bool convertIntPseudo(MachineInstr &MI, MachineBasicBlock &MBB);
  bool expandCall(MachineInstr &MI, MachineBasicBlock &MBB);
  bool expandRet(MachineInstr &MI, MachineBasicBlock &MBB);
  bool expandI2F(MachineInstr &MI, MachineBasicBlock &MBB);
  bool expandF2UI(MachineInstr &MI, MachineBasicBlock &MBB);
  bool expandF2SI(MachineInstr &MI, MachineBasicBlock &MBB);
};

} // namespace

PreservedAnalyses IC10ExpandPseudoInstsPassImpl::apply(MachineFunction &MF) {
  LLVMContext &Context = MF.getFunction().getContext();
  DoubleTy = Type::getDoubleTy(Context);

  bool Changed = false;
  for (MachineBasicBlock &MBB : MF) {
    auto MII = MBB.begin();
    while (MII != MBB.end()) {
      MachineInstr &MI = *MII++;
      Changed |= convertIntPseudo(MI, MBB);
      switch (MI.getOpcode()) {
      default:
        break;
      case IC10::CALL_PSEUDO:
        Changed |= expandCall(MI, MBB);
        break;
      case IC10::RET_PSEUDO:
        Changed |= expandRet(MI, MBB);
        break;
      case IC10::P_SINT2FP:
      case IC10::P_UINT2FP:
        Changed |= expandI2F(MI, MBB);
        break;
      }
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

namespace {

using namespace IC10;
const DenseMap<uint, uint> I2FOpcodeTable = {
    // clang-format off
    // move
    { IMOVEi, MOVEi }, { IMOVEr, MOVEr },

    // int arith
    { IADDrr, ADDrr }, { IADDri, ADDri },
    { ISUBrr, SUBrr }, { ISUBri, SUBri }, { ISUBir, SUBir },
    { IANDrr, ANDrr }, { IANDri, ANDri },
    { IORrr, ORrr }, { IORri, ORri },
    { INORrr, NORrr }, { INORri, NORri },
    { IXORrr, XORrr }, { IXORri, XORri },
    { ISLLrr, SLLrr }, { ISLLri, SLLri }, { ISLLir, SLLir },
    { ISRArr, SRArr }, { ISRAri, SRAri }, { ISRAir, SRAir },
    { ISRLrr, SRLrr }, { ISRLri, SRLri }, { ISRLir, SRLir },
    { INOTr, NOTr }, { IABSr, ABSr },

    // branch & set
    { BEQIrr, BEQrr }, { BEQIri, BEQri },
    { BNEIrr, BNErr }, { BNEIri, BNEri },
    { BGEIrr, BGErr }, { BGEIri, BGEri },
    { BGTIrr, BGTrr }, { BGTIri, BGTri },
    { BLEIrr, BLErr }, { BLEIri, BLEri },
    { BLTIrr, BLTrr }, { BLTIri, BLTri },
    { SEQIrr, SEQrr }, { SEQIri, SEQri },
    { SNEIrr, SNErr }, { SNEIri, SNEri },
    { SGEIrr, SGErr }, { SGEIri, SGEri },
    { SGTIrr, SGTrr }, { SGTIri, SGTri },
    { SLEIrr, SLErr }, { SLEIri, SLEri },
    { SLTIrr, SLTrr }, { SLTIri, SLTri },

    // select
    { ISELECTrr, SELECTrr },
    { ISELECTri, SELECTri },
    { ISELECTir, SELECTir },
    { ISELECTri, SELECTri },

    // load and store
    { ILDr, LDr }, { ILDi, LDi },
    { ISTrr, STrr }, { ISTir, STir }, { ISTri, STri }, { ISTii, STii },
    // clang-format on
};

} // namespace

bool IC10ExpandPseudoInstsPassImpl::convertIntPseudo(MachineInstr &MI,
                                                     MachineBasicBlock &MBB) {
  auto TableItr = I2FOpcodeTable.find(MI.getOpcode());
  if (TableItr == I2FOpcodeTable.end())
    return false;
  unsigned int NewOpcode = TableItr->getSecond();
  MI.setDesc(TII.get(NewOpcode));
  for (auto &MO : MI.operands()) {
    if (MO.isImm()) {
      int64_t Value = MO.getImm();
      assert(abs(Value) < (1L << 53));
      double FValue = (double)Value;
      ConstantFP *Constant = ConstantFP::get(DoubleTy, FValue);
      MO.ChangeToFPImmediate(Constant);
    }
  }

  return true;
}

bool IC10ExpandPseudoInstsPassImpl::expandCall(MachineInstr &MI,
                                               MachineBasicBlock &MBB) {
  assert(MI.getOpcode() == IC10::CALL_PSEUDO);

  DebugLoc DL = MI.getDebugLoc();
  MachineOperand &Target = MI.getOperand(0);

  BuildMI(MBB, MI, DL, TII.get(IC10::JALi)).add(Target);
  MI.eraseFromParent();
  return true;
}

bool IC10ExpandPseudoInstsPassImpl::expandRet(MachineInstr &MI,
                                              MachineBasicBlock &MBB) {
  assert(MI.getOpcode() == IC10::RET_PSEUDO);
  DebugLoc DL = MI.getDebugLoc();
  BuildMI(MBB, MI, DL, TII.get(IC10::Jr)).addReg(IC10::RA);
  MI.eraseFromParent();
  return true;
}

bool IC10ExpandPseudoInstsPassImpl::expandI2F(MachineInstr &MI,
                                              MachineBasicBlock &MBB) {
  assert(MI.getOpcode() == IC10::P_UINT2FP ||
         MI.getOpcode() == IC10::P_SINT2FP);
  DebugLoc DL = MI.getDebugLoc();
  Register Dst = MI.getOperand(0).getReg();
  Register Src = MI.getOperand(1).getReg();
  if (Src != Dst) {
    BuildMI(MBB, MI, DL, TII.get(IC10::MOVEr), Dst).addReg(Src);
  }
  MI.eraseFromParent();
  return true;
}

PreservedAnalyses
IC10ExpandPseudoInstsPass::run(MachineFunction &MF,
                               MachineFunctionAnalysisManager &MFAM) {
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  IC10ExpandPseudoInstsPassImpl Impl{MRI, TII};
  return Impl.apply(MF);
}
