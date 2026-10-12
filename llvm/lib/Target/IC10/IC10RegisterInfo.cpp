// IC10
#include "IC10RegisterInfo.h"
#include "IC10.h"
#include "IC10MachineFunctionInfo.h"
#include "IC10Subtarget.h"
#include "MCTargetDesc/IC10MCTargetDesc.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "IC10GenRegisterInfo.inc"

IC10RegisterInfo::IC10RegisterInfo() : IC10GenRegisterInfo(IC10::RA) {}

const MCPhysReg *
IC10RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CC_Save_SaveList;
}

const uint32_t *
IC10RegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const {
  return CC_Save_RegMask;
}

BitVector IC10RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  Reserved.set(IC10::RA);
  Reserved.set(IC10::R15);

  return Reserved;
}

bool IC10RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                           int SPAdj, unsigned FIOperandNum,
                                           RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected non-zero SPAdj value");

  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  const IC10MachineFunctionInfo *MFI = MF.getInfo<IC10MachineFunctionInfo>();

  int FI = MI.getOperand(FIOperandNum).getIndex();
  int Offset = MFI->calculateStackSlotOffset(MF, FI);
  if (Offset < 0) {
    report_fatal_error("Negative frame index offset!");
  }
  // IC10 TODO
  switch (MI.getOpcode()) {
  case IC10::IADDri:
    eliminateFrameIndexInADDri(II, FIOperandNum, Offset);
    return true;
  //   return true;
  // case IC10::ADD_I_RI:
  //   eliminateFrameIndexInADDRI(II, FIOperandNum, Offset);
  //   return true;
  default:
    llvm_unreachable("Frame index cannot be eliminated for opcode ");
  }
  return false;
}

Register IC10RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return IC10::R15;
}

//===----------------------------------------------------------------------===//
// Eliminate Frame Index
//===----------------------------------------------------------------------===//

void IC10RegisterInfo::eliminateFrameIndexInADDri(
    MachineBasicBlock::iterator II, unsigned FIOperandNum, int Offset) const {
  assert(II->getOpcode() == IC10::IADDri);
  assert(FIOperandNum == 1 && "Bad inst in eliminateFrameIndex.");
  assert(Offset % 8 == 0 && "Alignment failure.");
  MachineInstr &MI = *II;
  const int64_t OldOffset = MI.getOperand(2).getImm();
  const int64_t NewOffset = OldOffset + Offset / 8;
  MI.getOperand(1).ChangeToRegister(IC10::R15, false);
  MI.getOperand(2).ChangeToImmediate(NewOffset);
}
