// IC10
#include "TargetInfo/IC10TargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheIC10Target() {
  static Target TheIC10Target;
  return TheIC10Target;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeIC10TargetInfo() {
  RegisterTarget<Triple::ic10, /*HasJIT=*/false> X(getTheIC10Target(), "ic10",
                                                  "IC10 CPU", "IC10");
}
