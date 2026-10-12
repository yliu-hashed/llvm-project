// IC10
#ifndef LLVM_LIB_TARGET_IC10_IC10REGISTERINFO_H
#define LLVM_LIB_TARGET_IC10_IC10REGISTERINFO_H

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/MC/MCRegister.h"

#define GET_REGINFO_HEADER
#include "IC10GenRegisterInfo.inc"

namespace llvm {
struct IC10RegisterInfo : public IC10GenRegisterInfo {
  IC10RegisterInfo();

public:
  const uint16_t *
  getCalleeSavedRegs(const MachineFunction *MF = 0) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;

  bool eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  Register getFrameRegister(const MachineFunction &MF) const override;

  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const override;

private:
  void eliminateFrameIndexInADDri(MachineBasicBlock::iterator II,
                                  unsigned FIOperandNum, int Offset) const;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_IC10_IC10REGISTERINFO_H
