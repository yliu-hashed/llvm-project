// IC10
#ifndef LLVM_LIB_TARGET_IC10_IC10INSTRINFO_H
#define LLVM_LIB_TARGET_IC10_IC10INSTRINFO_H

#include "IC10.h"
#include "IC10RegisterInfo.h"
#include "MCTargetDesc/IC10MCTargetDesc.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "IC10GenInstrInfo.inc"

namespace llvm {

class IC10Subtarget;

namespace IC10CC {

enum CondCode {
  COND_INVALID = 0,
  COND_IEQ,
  COND_INE,
  COND_IGE,
  COND_IGT,
  COND_ILE,
  COND_ILT,
  COND_ICMP_FIRST = COND_IEQ,
  COND_ICMP_LAST = COND_ILT,
  COND_FEQ,
  COND_FNE,
  COND_FGE,
  COND_FGT,
  COND_FLE,
  COND_FLT,
  COND_FNAN,
  COND_FNANZ,
  COND_FCMP_FIRST = COND_FEQ,
  COND_FCMP_LAST = COND_FNANZ,
};

inline bool isFloat(CondCode CC) {
  return CC >= COND_FCMP_FIRST && CC <= COND_FCMP_LAST;
}

inline bool isSingle(CondCode CC) {
  return CC == COND_FNAN || CC == COND_FNANZ;
}

CondCode flipped(CondCode CC);
CondCode negated(CondCode CC);

} // namespace IC10CC

struct IC10InstrInfo : public IC10GenInstrInfo {
private:
  const IC10RegisterInfo RI;
  const IC10Subtarget &Subtarget;
  virtual void anchor();

public:
  explicit IC10InstrInfo(IC10Subtarget &ST);

  const IC10RegisterInfo &getRegisterInfo() const { return RI; }

  unsigned int getBrCCOpcode(IC10CC::CondCode CC, bool RHSIsImm) const;
  IC10CC::CondCode getBrOpcodeCC(unsigned int Opcode) const;
  unsigned int getCmpOpcode(IC10CC::CondCode CC, bool RHSIsImm) const;

  bool useMachineCombiner() const override { return true; }
  // bool isAssociativeAndCommutative(const MachineInstr &Inst,
  //                                  bool Invert) const override;

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                   const DebugLoc &DL, Register DestReg, Register SrcReg,
                   bool KillSrc, bool RenamableDest = false,
                   bool RenamableSrc = false) const override;

  void storeRegToStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
      bool isKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;

  void loadRegFromStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register DestReg,
      int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      unsigned SubReg = 0,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;

  bool isUnpredicatedTerminator(const MachineInstr &MI) const;

  bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify) const override;

  unsigned removeBranch(MachineBasicBlock &MBB,
                        int *BytesRemoved = nullptr) const override;

  unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
                        const DebugLoc &DL,
                        int *BytesAdded = nullptr) const override;

  bool
  reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const override;

  MachineBasicBlock *getBranchDestBlock(const MachineInstr &MI) const override;
};

} // namespace llvm

#endif
