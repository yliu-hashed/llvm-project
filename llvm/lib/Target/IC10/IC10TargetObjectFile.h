// IC10
#ifndef LLVM_LIB_TARGET_IC10_IC10TARGETOBJECTFILE_H
#define LLVM_LIB_TARGET_IC10_IC10TARGETOBJECTFILE_H

#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm {

class MCContext;
class TargetMachine;

class IC10TargetObjectFile : public TargetLoweringObjectFileELF {
  void Initialize(MCContext &Ctx, const TargetMachine &TM) override;
};

} // end namespace llvm

#endif
