// IC10

#include "IC10TargetLowering.h"
#include "IC10RegisterInfo.h"
#include "IC10TargetMachine.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/Support/KnownBits.h"

using namespace llvm;

IC10TargetLowering::IC10TargetLowering(const IC10TargetMachine &TM,
                                       const IC10Subtarget &STI)
    : TargetLowering(TM, STI), Subtarget(STI), TM(TM) {
  setBooleanContents(ZeroOrOneBooleanContent);
  addRegisterClass(MVT::i64, &IC10::GPRRegClass);
  addRegisterClass(MVT::f64, &IC10::FPRRegClass);
  computeRegisterProperties(STI.getRegisterInfo());
}

#define GET_CALLING_CONV_IMPL
#include "IC10GenCallingConv.inc"

CCAssignFn *IC10TargetLowering::getCCAssignFn(bool Return) const {
  if (Return)
    return RetCC_IC10;
  return CC_IC10;
}

void IC10TargetLowering::computeKnownBitsForTargetInstr(
    GISelValueTracking &Analysis, Register R, KnownBits &Known,
    const APInt &DemandedElts, const MachineRegisterInfo &MRI,
    unsigned int Depth) const {

  Known.resetAll();

  const MachineInstr *MI = MRI.getVRegDef(R);
  switch (MI->getOpcode()) {
  default:
    return;
  case IC10::G_FEQ:
  case IC10::G_FNE:
  case IC10::G_FGE:
  case IC10::G_FGT:
  case IC10::G_FLE:
  case IC10::G_FLT:
    Known.Zero.setBitsFrom(1);
    return;
  }
}
