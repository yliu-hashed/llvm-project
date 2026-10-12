// IC10
#include "IC10MachineFunctionInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"

using namespace llvm;

void IC10MachineFunctionInfo::anchor() {}

uint64_t
IC10MachineFunctionInfo::calculateStackSize(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  uint64_t StackSize = MFI.getStackSize();

  assert(!MF.getFrameInfo().hasVarSizedObjects() &&
         "Var sized objects are not supported!");
  // no need to store return address, RA is saved by the spill system
  return StackSize;
}

uint64_t
IC10MachineFunctionInfo::calculateStackSlotOffset(const MachineFunction &MF,
                                                  int FrameIndex) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return MFI.getObjectOffset(FrameIndex) + MFI.getStackSize();
}
