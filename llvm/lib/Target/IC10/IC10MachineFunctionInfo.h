// IC10
#ifndef LLVM_LIB_TARGET_IC10_IC10MACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_IC10_IC10MACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

class IC10MachineFunctionInfo : public MachineFunctionInfo {
  virtual void anchor();

public:
  uint64_t calculateStackSize(const MachineFunction &MF) const;
  uint64_t calculateStackSlotOffset(const MachineFunction &MF,
                                    int FrameIndex) const;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_IC10_IC10MACHINEFUNCTIONINFO_H
