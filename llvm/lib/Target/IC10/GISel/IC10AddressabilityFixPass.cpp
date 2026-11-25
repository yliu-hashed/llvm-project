// IC10

#include "IC10.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/IR/Analysis.h"
#include "llvm/Target/TargetMachine.h"
#include <cassert>

using namespace llvm;

namespace {

struct IC10PreLegalizerAddressabilityFixPassImpl {

  MachineRegisterInfo &MRI;
  const TargetInstrInfo &TII;

  PreservedAnalyses apply(MachineFunction &MF);

private:
  bool fixPtrAdd(MachineInstr &MI, MachineIRBuilder &MIRBuilder);
  bool fixPtr2Int(MachineInstr &MI, MachineIRBuilder &MIRBuilder);
  bool fixInt2Ptr(MachineInstr &MI, MachineIRBuilder &MIRBuilder);
};

} // namespace

PreservedAnalyses
IC10PreLegalizerAddressabilityFixPassImpl::apply(MachineFunction &MF) {
  MachineIRBuilder MIRBuilder(MF);

  bool Changed = false;
  for (MachineBasicBlock &MBB : MF) {
    auto MII = MBB.begin();
    while (MII != MBB.end()) {
      MachineInstr &MI = *MII++;
      switch (MI.getOpcode()) {
      default:
        break;
      case TargetOpcode::G_PTR_ADD:
        MIRBuilder.setInsertPt(MBB, MI);
        Changed |= fixPtrAdd(MI, MIRBuilder);
        break;
      case TargetOpcode::G_INTTOPTR:
        MIRBuilder.setInsertPt(MBB, MI);
        Changed |= fixInt2Ptr(MI, MIRBuilder);
        break;
      case TargetOpcode::G_PTRTOINT:
        MIRBuilder.setInsertPt(MBB, MI);
        Changed |= fixPtr2Int(MI, MIRBuilder);
        break;
      }
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool IC10PreLegalizerAddressabilityFixPassImpl::fixPtrAdd(
    MachineInstr &MI, MachineIRBuilder &MIRBuilder) {
  using namespace TargetOpcode;
  assert(MI.getOpcode() == G_PTR_ADD);

  MachineOperand &Offset = MI.getOperand(2);
  Register ByteOffsetReg = Offset.getReg();
  LLT Type = MRI.getType(ByteOffsetReg);

  Register ShiftAmount = MRI.createGenericVirtualRegister(Type);
  MIRBuilder.buildConstant(ShiftAmount, 3);
  Register Shifted = MRI.createGenericVirtualRegister(Type);
  MIRBuilder.buildAShr(Shifted, ByteOffsetReg, ShiftAmount);
  Offset.ChangeToRegister(Shifted, false);
  return true;
}

bool IC10PreLegalizerAddressabilityFixPassImpl::fixPtr2Int(
    MachineInstr &MI, MachineIRBuilder &MIRBuilder) {
  using namespace TargetOpcode;
  assert(MI.getOpcode() == G_PTRTOINT);

  MachineOperand &Result = MI.getOperand(0);
  Register ResultReg = Result.getReg();
  LLT Type = MRI.getType(ResultReg);

  Register ByteResultReg = MRI.createGenericVirtualRegister(Type);
  Result.ChangeToRegister(ByteResultReg, true);

  Register ShiftAmount = MRI.createGenericVirtualRegister(Type);
  MIRBuilder.buildConstant(ShiftAmount, 3);
  MIRBuilder.buildShl(ResultReg, ByteResultReg, ShiftAmount);
  return true;
}

bool IC10PreLegalizerAddressabilityFixPassImpl::fixInt2Ptr(
    MachineInstr &MI, MachineIRBuilder &MIRBuilder) {
  using namespace TargetOpcode;
  assert(MI.getOpcode() == G_INTTOPTR);

  MachineOperand &Source = MI.getOperand(1);
  Register ByteSourceReg = Source.getReg();
  LLT Type = MRI.getType(ByteSourceReg);

  Register Shifted = MRI.createGenericVirtualRegister(Type);
  Register ShiftAmount = MRI.createGenericVirtualRegister(Type);
  MIRBuilder.buildConstant(ShiftAmount, 3);
  MIRBuilder.buildLShr(Shifted, ByteSourceReg, ShiftAmount);
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
