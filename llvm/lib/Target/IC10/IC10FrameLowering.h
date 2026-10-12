// IC10
#ifndef LLVM_LIB_TARGET_IC10_IC10FRAMELOWERING_H
#define LLVM_LIB_TARGET_IC10_IC10FRAMELOWERING_H

#include "IC10.h"
#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class IC10Subtarget;
class IC10FrameLowering : public TargetFrameLowering {
public:
  explicit IC10FrameLowering(const IC10Subtarget &ST);

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  MachineBasicBlock::iterator
  eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I) const override;

  bool hasFPImpl(const MachineFunction &MF) const override { return false; }

  bool canSimplifyCallFramePseudos(const MachineFunction &MF) const override;

private:
};

} // namespace llvm

#endif
