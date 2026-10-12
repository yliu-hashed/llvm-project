// IC10
#include "IC10Subtarget.h"
#include "GISel/IC10CallLowering.h"
#include "GISel/IC10LegalizerInfo.h"
#include "GISel/IC10RegisterBankInfo.h"
#include "IC10.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "IC10-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "IC10GenSubtargetInfo.inc"

void IC10Subtarget::anchor() {}

IC10Subtarget::IC10Subtarget(const Triple &TT, StringRef CPU, StringRef FS,
                             const IC10TargetMachine &TM)
    : IC10GenSubtargetInfo(TT, CPU, /*TUNE CPU*/ CPU, FS), TM(TM),
      InstrInfo(*this), TLInfo(TM, *this), FrameLowering(*this) {
  CallLoweringInfo.reset(new IC10CallLowering(*getTargetLowering()));
  Legalizer.reset(new IC10LegalizerInfo(*this));
  auto *RBI = new IC10RegisterBankInfo(*getRegisterInfo());
  RegBankInfo.reset(RBI);
  InstSelector.reset(createIC10InstructionSelector(TM, *this, *RBI));
}

const CallLowering *IC10Subtarget::getCallLowering() const {
  return CallLoweringInfo.get();
}

InstructionSelector *IC10Subtarget::getInstructionSelector() const {
  return InstSelector.get();
}

const LegalizerInfo *IC10Subtarget::getLegalizerInfo() const {
  return Legalizer.get();
}

const RegisterBankInfo *IC10Subtarget::getRegBankInfo() const {
  return RegBankInfo.get();
}
