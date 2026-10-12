// IC10
#ifndef LLVM_LIB_TARGET_IC10_MCTARGETDESC_IC10MCASMINFO_H
#define LLVM_LIB_TARGET_IC10_MCTARGETDESC_IC10MCASMINFO_H

#include "llvm/MC/MCAsmInfo.h"

namespace llvm {

class Triple;

class IC10MCAsmInfo : public MCAsmInfo {
  virtual void anchor();

public:
  explicit IC10MCAsmInfo(const MCTargetOptions &Options);
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_IC10_MCTARGETDESC_IC10MCASMINFO_H
