// IC10
#include "IC10FrameLowering.h"
#include "IC10InstrInfo.h"
#include "IC10MachineFunctionInfo.h"
#include "IC10Subtarget.h"
#include "MCTargetDesc/IC10MCTargetDesc.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Target/TargetOptions.h"
#include <algorithm>
#include <cassert>

using namespace llvm;

IC10FrameLowering::IC10FrameLowering(const IC10Subtarget &ST)
    : TargetFrameLowering(TargetFrameLowering::StackGrowsDown,
                          /*StackAlignment=*/Align(8),
                          /*LAO=*/0,
                          /*TransAL=*/Align(8),
                          /*StackReal*/ false) {}

void IC10FrameLowering::emitPrologue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  // Compute the stack size, to determine if we need a prologue at all.
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  const IC10MachineFunctionInfo *MFI = MF.getInfo<IC10MachineFunctionInfo>();
  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  int64_t StackSize = MFI->calculateStackSize(MF);
  if (!StackSize) return;
  assert(StackSize % 8 == 0 && "Bad stack alignment!");

  // Adjust the stack register
  BuildMI(MBB, MBBI, DL, TII.get(IC10::IADDri), IC10::R15)
    .addReg(IC10::R15)
    .addImm(-StackSize / 8)
    .setMIFlag(MachineInstr::FrameSetup);

  for (MachineBasicBlock &MBB : MF) {
    MBB.addLiveIn(IC10::R15);
  }
}

void IC10FrameLowering::emitEpilogue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  // Compute the stack size, to determine if we need an epilogue at all
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  const IC10MachineFunctionInfo *MFI = MF.getInfo<IC10MachineFunctionInfo>();
  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  DebugLoc DL = MBBI->getDebugLoc();

  int64_t StackSize = MFI->calculateStackSize(MF);
  if (!StackSize) return;
  assert(StackSize % 8 == 0 && "Bad stack alignment!");

  // Restore the stack register to what it was at the beginning of the function
  BuildMI(MBB, MBBI, DL, TII.get(IC10::IADDri), IC10::R15)
    .addReg(IC10::R15)
    .addImm(StackSize / 8)
    .setMIFlag(MachineInstr::FrameDestroy);
}

bool IC10FrameLowering::canSimplifyCallFramePseudos(
    const MachineFunction &MF) const {
  return true;
}

MachineBasicBlock::iterator IC10FrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  return MBB.erase(I);
}
