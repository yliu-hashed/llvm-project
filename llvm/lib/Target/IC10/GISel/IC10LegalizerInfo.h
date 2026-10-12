// IC10

#ifndef LLVM_LIB_TARGET_IC10_GLSEL_IC10LEGALIZERINFO_H
#define LLVM_LIB_TARGET_IC10_GLSEL_IC10LEGALIZERINFO_H

#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"

namespace llvm {

class IC10Subtarget;

struct IC10LegalizerInfo : public LegalizerInfo {
public:
  IC10LegalizerInfo(const IC10Subtarget &ST);
  bool legalizeCustom(LegalizerHelper &Helper, MachineInstr &MI,
                      LostDebugLocObserver &LocObserver) const override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_IC10_GLSEL_IC10LEGALIZERINFO_H
