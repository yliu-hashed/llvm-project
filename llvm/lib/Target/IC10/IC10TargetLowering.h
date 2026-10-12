// IC10

#ifndef LLVM_LIB_TARGET_IC10_IC10TARGETLOWERING_H
#define LLVM_LIB_TARGET_IC10_IC10TARGETLOWERING_H

#include "IC10.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/IR/CallingConv.h"

namespace llvm {
class IC10Subtarget;
struct IC10RegisterInfo;

class IC10TargetLowering : public TargetLowering {
  const IC10Subtarget &Subtarget;
  const IC10TargetMachine &TM;

public:
  IC10TargetLowering(const IC10TargetMachine &TM, const IC10Subtarget &STI);

  bool isSuitableForJumpTable(const SwitchInst *SI, uint64_t NumCases,
                              uint64_t Range, ProfileSummaryInfo *PSI,
                              BlockFrequencyInfo *BFI) const override {
    return false;
  }

  CCAssignFn *getCCAssignFn(bool Return) const;

  void computeKnownBitsForTargetInstr(GISelValueTracking &Analysis, Register R,
                                      KnownBits &Known,
                                      const APInt &DemandedElts,
                                      const MachineRegisterInfo &MRI,
                                      unsigned int Depth = 0) const override;
};
} // end namespace llvm

#endif // LLVM_LIB_TARGET_IC10_IC10TARGETLOWERING_H
