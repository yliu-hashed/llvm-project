// IC10
#include "IC10MCTargetDesc.h"
#include "IC10InstPrinter.h"
#include "IC10MCAsmInfo.h"
#include "TargetInfo/IC10TargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/MC/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "IC10GenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "IC10GenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "IC10GenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createIC10MCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitIC10MCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createIC10MCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitIC10MCRegisterInfo(X, IC10::RA);
  return X;
}

static MCAsmInfo *createIC10MCAsmInfo(const MCRegisterInfo &MRI,
                                      const Triple &TT,
                                      const MCTargetOptions &Options) {
  return new IC10MCAsmInfo(Options);
}

static MCSubtargetInfo *createIC10MCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  return createIC10MCSubtargetInfoImpl(TT, CPU, /*TuneCPU*/CPU, FS);
}

static MCInstPrinter *createIC10MCInstPrinter(const Triple &TT, unsigned SyntaxVariant,
                       const MCAsmInfo &MAI, const MCInstrInfo &MII,
                       const MCRegisterInfo &MRI) {
  return new IC10InstPrinter(MAI, MII, MRI);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeIC10TargetMC() {
  RegisterMCAsmInfoFn X(getTheIC10Target(), createIC10MCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(getTheIC10Target(), createIC10MCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(getTheIC10Target(), createIC10MCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(getTheIC10Target(), createIC10MCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(getTheIC10Target(), createIC10MCInstPrinter);
}
