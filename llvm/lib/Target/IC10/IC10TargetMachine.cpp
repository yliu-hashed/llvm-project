// IC10

#include "IC10TargetMachine.h"
#include "IC10.h"
#include "IC10TargetObjectFile.h"
#include "TargetInfo/IC10TargetInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeIC10Target() {
  // Register the target.
  RegisterTargetMachine<IC10TargetMachine> X(getTheIC10Target());

  // auto &PR = *PassRegistry::getPassRegistry();
  // initializeIC10AsmPrinterPass(PR);
}

IC10TargetMachine::IC10TargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     std::optional<Reloc::Model> RM,
                                     std::optional<CodeModel::Model> CM,
                                     CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(T, TT, CPU, FS, Options,
                               RM.value_or(Reloc::Static),
                               CM.value_or(CodeModel::Large), OL),
      TLOF(std::make_unique<IC10TargetObjectFile>()),
      Subtarget(TT, std::string(CPU), std::string(FS), *this) {
  initAsmInfo();
  setGlobalISelAbort(GlobalISelAbortMode::Enable);
  setGlobalISel(true);
}

IC10TargetMachine::~IC10TargetMachine() {}

const IC10Subtarget *
IC10TargetMachine::getSubtargetImpl(const Function &F) const {
  return &Subtarget;
}
