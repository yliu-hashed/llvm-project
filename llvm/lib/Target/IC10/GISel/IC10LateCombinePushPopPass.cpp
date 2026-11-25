// IC10

#include "IC10.h"
#include "MCTargetDesc/IC10MCTargetDesc.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Target/TargetMachine.h"
#include <_types/_nl_item.h>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdint.h>
#include <sys/types.h>

using namespace llvm;

namespace {

struct IC10LateCombinePushPopPassImpl {
  MachineRegisterInfo &MRI;
  const TargetInstrInfo &TII;

  IC10LateCombinePushPopPassImpl(MachineRegisterInfo &MRI,
                                 const TargetInstrInfo &TII)
      : MRI(MRI), TII(TII) {}

  PreservedAnalyses apply(MachineFunction &MF);

private:
  Type *DoubleTy = nullptr;

  struct OperandAndOffset {
    MachineOperand Operand;
    int64_t Offset;
    DebugLoc DL;
    static bool ascending(const OperandAndOffset &LHS,
                          const OperandAndOffset &RHS) {
      return LHS.Offset < RHS.Offset;
    }
    static bool decending(const OperandAndOffset &LHS,
                          const OperandAndOffset &RHS) {
      return LHS.Offset > RHS.Offset;
    }
  };

  bool rewrite(MachineBasicBlock::iterator &MII,
               MachineBasicBlock::iterator End, MachineBasicBlock &MBB);

  bool isAddR15ToSP(const MachineInstr &MI, int64_t &Offset) {
    unsigned int Opcode = MI.getOpcode();
    if (Opcode != IC10::ADDri)
      return false;
    if (MI.getOperand(0).getReg() != IC10::SP)
      return false;
    if (MI.getOperand(1).getReg() != IC10::R15)
      return false;
    const MachineOperand &OffsetOp = MI.getOperand(2);
    assert(OffsetOp.isFPImm());
    double OffsetF = OffsetOp.getFPImm()->getValue().convertToDouble();
    // must be exact integer
    assert(OffsetF - trunc(OffsetF) == 0);
    assert(!isnan(OffsetF) && !isinf(OffsetF));
    Offset = (int64_t)OffsetF;
    return true;
  }

  bool getSTOperand(const MachineInstr &MI, MachineOperand &Operand) {
    unsigned int Opcode = MI.getOpcode();
    if (Opcode != IC10::PUSHi && Opcode != IC10::PUSHr) {
      return false;
    }
    Operand = MI.getOperand(0);
    if (Operand.isReg()) {
      Register Reg = Operand.getReg();
      if (Reg == IC10::SP) return false;
    }
    return true;
  }

  bool getLDOperand(const MachineInstr &MI, MachineOperand &Operand) {
    unsigned int Opcode = MI.getOpcode();
    if (Opcode != IC10::PEEK)
      return false;
    Operand = MI.getOperand(0);
    if (Operand.isReg()) {
      Register Reg = Operand.getReg();
      if (Reg == IC10::SP || Reg == IC10::R15) return false;
    }
    return true;
  }

  bool rewriteLDSequence(MachineBasicBlock::iterator Begin,
                         MachineBasicBlock::iterator End,
                         MachineBasicBlock &MBB,
                         SmallVector<OperandAndOffset, 4> &Items,
                         int64_t FinalOffset);
  bool rewriteSTSequence(MachineBasicBlock::iterator Begin,
                         MachineBasicBlock::iterator End,
                         MachineBasicBlock &MBB,
                         SmallVector<OperandAndOffset, 4> &Items,
                         int64_t FinalOffset);
};

} // namespace

bool IC10LateCombinePushPopPassImpl::rewrite(MachineBasicBlock::iterator &MII,
                                             MachineBasicBlock::iterator End,
                                             MachineBasicBlock &MBB) {
  MachineBasicBlock::iterator RegionBegin = MII;
  MachineBasicBlock::iterator RegionEnd = End;
  SmallVector<OperandAndOffset, 4> Items;
  MachineInstr &FirstMI = *MII++;
  int64_t Offset = 0;
  if ((MII == End) || !isAddR15ToSP(FirstMI, Offset))
    return false;
  MachineInstr &SecondMI = *MII;
  MachineOperand Operand = MachineOperand::CreateImm(0); // place holder
  if (getLDOperand(SecondMI, Operand)) {
    RegionEnd = ++MII;
    Items.push_back({Operand, Offset - 1, SecondMI.getDebugLoc()});
    while (MII != End) {
      MachineInstr &MIA = *MII++;
      if (MII == End || !isAddR15ToSP(MIA, Offset))
        break;
      MachineInstr &MIB = *MII;
      if (!getLDOperand(MIB, Operand))
        break;
      Items.push_back({Operand, Offset - 1, MIB.getDebugLoc()});
      RegionEnd = ++MII;
    }
    const int64_t LastOffset = Items.back().Offset + 1;
    return rewriteLDSequence(RegionBegin, RegionEnd, MBB, Items, LastOffset);
  }
  if (getSTOperand(SecondMI, Operand)) {
    RegionEnd = ++MII;
    Items.push_back({Operand, Offset, SecondMI.getDebugLoc()});
    while (MII != End) {
      MachineInstr &MIA = *MII++;
      if (MII == End || !isAddR15ToSP(MIA, Offset))
        break;
      MachineInstr &MIB = *MII;
      if (!getSTOperand(MIB, Operand))
        break;
      Items.push_back({Operand, Offset, MIB.getDebugLoc()});
      RegionEnd = ++MII;
    }
    const int64_t LastOffset = Items.back().Offset + 1;
    return rewriteSTSequence(RegionBegin, RegionEnd, MBB, Items, LastOffset);
  }
  return false;
}

bool IC10LateCombinePushPopPassImpl::rewriteLDSequence(
    MachineBasicBlock::iterator Begin, MachineBasicBlock::iterator End,
    MachineBasicBlock &MBB, SmallVector<OperandAndOffset, 4> &Items,
    int64_t FinalOffset) {
  if (Items.size() <= 1)
    return false;
  sort(Items, OperandAndOffset::decending);
  // rewrite
  MBB.erase(Begin, End);
  int64_t CurrentSPOffset = INT64_MAX;
  for (auto &Item : Items) {
    if (CurrentSPOffset != Item.Offset + 1) {
      BuildMI(MBB, End, Item.DL, TII.get(IC10::ADDri), IC10::SP)
          .addReg(IC10::R15)
          .addFPImm(ConstantFP::get(DoubleTy, Item.Offset + 1));
      CurrentSPOffset = Item.Offset + 1;
    }
    BuildMI(MBB, End, Item.DL, TII.get(IC10::POP)).add(Item.Operand);
    CurrentSPOffset--;
  }
  if (FinalOffset != CurrentSPOffset) {
    BuildMI(MBB, End, Items.back().DL, TII.get(IC10::ADDri), IC10::SP)
        .addReg(IC10::R15)
        .addFPImm(ConstantFP::get(DoubleTy, FinalOffset));
  }
  return true;
}

bool IC10LateCombinePushPopPassImpl::rewriteSTSequence(
    MachineBasicBlock::iterator Begin, MachineBasicBlock::iterator End,
    MachineBasicBlock &MBB, SmallVector<OperandAndOffset, 4> &Items,
    int64_t FinalOffset) {
  if (Items.size() <= 1)
    return false;
  sort(Items, OperandAndOffset::ascending);
  // rewrite
  MBB.erase(Begin, End);
  int64_t CurrentSPOffset = INT64_MAX;
  for (auto &Item : Items) {
    if (CurrentSPOffset != Item.Offset) {
      BuildMI(MBB, End, Item.DL, TII.get(IC10::ADDri), IC10::SP)
          .addReg(IC10::R15)
          .addFPImm(ConstantFP::get(DoubleTy, Item.Offset));
      CurrentSPOffset = Item.Offset;
    }
    unsigned int Opcode = Item.Operand.isFPImm() ? IC10::PUSHi : IC10::PUSHr;
    BuildMI(MBB, End, Item.DL, TII.get(Opcode)).add(Item.Operand);
    CurrentSPOffset++;
  }
  if (FinalOffset != CurrentSPOffset) {
    BuildMI(MBB, End, Items.back().DL, TII.get(IC10::ADDri), IC10::SP)
        .addReg(IC10::R15)
        .addFPImm(ConstantFP::get(DoubleTy, FinalOffset));
  }
  return true;
}

PreservedAnalyses IC10LateCombinePushPopPassImpl::apply(MachineFunction &MF) {
  LLVMContext &Context = MF.getFunction().getContext();
  DoubleTy = Type::getDoubleTy(Context);

  bool Changed = false;
  for (MachineBasicBlock &MBB : MF) {
    auto MII = MBB.begin();
    while (MII != MBB.end()) {
      Changed |= rewrite(MII, MBB.end(), MBB);
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PreservedAnalyses
IC10LateCombinePushPopPass::run(MachineFunction &MF,
                                MachineFunctionAnalysisManager &MFAM) {
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  IC10LateCombinePushPopPassImpl Impl{MRI, TII};
  return Impl.apply(MF);
}
