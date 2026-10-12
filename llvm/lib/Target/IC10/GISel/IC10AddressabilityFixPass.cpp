// IC10

#include "IC10.h"
#include "IC10TargetMachine.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Scalar/LowerAtomicPass.h"
#include <cassert>

using namespace llvm;

namespace {

struct IC10PreLegalizerAddressabilityFixPassImpl {

  MachineRegisterInfo &MRI;
  const TargetInstrInfo &TII;

  PreservedAnalyses apply(MachineFunction &MF);

private:
  bool fixPtrAdd(MachineInstr &MI, MachineBasicBlock &MBB);
  bool fixPtr2Int(MachineInstr &MI, MachineBasicBlock &MBB);
  bool fixInt2Ptr(MachineInstr &MI, MachineBasicBlock &MBB);
};

} // namespace

PreservedAnalyses
IC10PreLegalizerAddressabilityFixPassImpl::apply(MachineFunction &MF) {
  bool Changed = false;
  for (MachineBasicBlock &MBB : MF) {
    auto MII = MBB.begin();
    while (MII != MBB.end()) {
      MachineInstr &MI = *MII++;
      switch (MI.getOpcode()) {
      default:
        break;
      case TargetOpcode::G_PTR_ADD:
        Changed |= fixPtrAdd(MI, MBB);
        break;
      case TargetOpcode::G_INTTOPTR:
        Changed |= fixInt2Ptr(MI, MBB);
        break;
      case TargetOpcode::G_PTRTOINT:
        Changed |= fixPtr2Int(MI, MBB);
        break;
      }
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool IC10PreLegalizerAddressabilityFixPassImpl::fixPtrAdd(
    MachineInstr &MI, MachineBasicBlock &MBB) {
  using namespace TargetOpcode;
  assert(MI.getOpcode() == G_PTR_ADD);

  DebugLoc DL = MI.getDebugLoc();
  MachineOperand &Offset = MI.getOperand(2);
  Register ByteOffsetReg = Offset.getReg();
  LLT Type = MRI.getType(ByteOffsetReg);

  Register Shifted = MRI.createGenericVirtualRegister(Type);
  Register ShiftAmount = MRI.createGenericVirtualRegister(Type);
  BuildMI(MBB, MI, DL, TII.get(G_CONSTANT), ShiftAmount)
      .addImm(3);
  BuildMI(MBB, MI, DL, TII.get(G_LSHR), Shifted)
      .addReg(ByteOffsetReg)
      .addReg(ShiftAmount);
  Offset.ChangeToRegister(Shifted, false);
  return true;
}

bool IC10PreLegalizerAddressabilityFixPassImpl::fixPtr2Int(
    MachineInstr &MI, MachineBasicBlock &MBB) {
  using namespace TargetOpcode;
  assert(MI.getOpcode() == G_PTRTOINT);

  DebugLoc DL = MI.getDebugLoc();
  MachineOperand &Result = MI.getOperand(0);
  Register ResultReg = Result.getReg();
  LLT Type = MRI.getType(ResultReg);

  Register ByteResultReg = MRI.createGenericVirtualRegister(Type);
  Result.ChangeToRegister(ByteResultReg, true);

  Register ShiftAmount = MRI.createGenericVirtualRegister(Type);
  BuildMI(MBB, MI, DL, TII.get(G_CONSTANT), ShiftAmount).addImm(3);
  BuildMI(MBB, MI, DL, TII.get(G_SHL), ResultReg)
      .addReg(ByteResultReg)
      .addReg(ShiftAmount);
  return true;
}

bool IC10PreLegalizerAddressabilityFixPassImpl::fixInt2Ptr(
    MachineInstr &MI, MachineBasicBlock &MBB) {
  using namespace TargetOpcode;
  assert(MI.getOpcode() == G_INTTOPTR);

  DebugLoc DL = MI.getDebugLoc();
  MachineOperand &Source = MI.getOperand(1);
  Register ByteSourceReg = Source.getReg();
  LLT Type = MRI.getType(ByteSourceReg);

  Register Shifted = MRI.createGenericVirtualRegister(Type);
  Register ShiftAmount = MRI.createGenericVirtualRegister(Type);
  BuildMI(MBB, MI, DL, TII.get(G_CONSTANT), ShiftAmount)
      .addImm(3);
  BuildMI(MBB, MI, DL, TII.get(G_LSHR), Shifted)
      .addReg(ByteSourceReg)
      .addReg(ShiftAmount);
  Source.ChangeToRegister(Shifted, false);
  return true;
}

PreservedAnalyses IC10PreLegalizerAddressabilityFixPass::run(
    MachineFunction &MF, MachineFunctionAnalysisManager &MFAM) {
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  IC10PreLegalizerAddressabilityFixPassImpl Impl{MRI, TII};
  return Impl.apply(MF);
}
