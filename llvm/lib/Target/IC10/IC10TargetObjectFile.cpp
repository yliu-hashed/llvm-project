// LC3

#include "IC10TargetObjectFile.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

void IC10TargetObjectFile::initialize(MCContext &Ctx, const TargetMachine &TM,
                                      const Module &M) {
  TargetLoweringObjectFileELF::initialize(Ctx, TM, M);
}
