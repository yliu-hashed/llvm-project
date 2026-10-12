// IC10

#include "IC10.h"
#include "IC10TargetMachine.h"
#include "MCTargetDesc/IC10MCTargetDesc.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Pass.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/ModRef.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Scalar/LowerAtomicPass.h"
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

  bool consume(MachineBasicBlock::iterator &MII,
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

  bool isStackSTOperand(const MachineInstr &MI, MachineOperand &Operand) {
    unsigned int Opcode = MI.getOpcode();
    if (Opcode == IC10::STrr) {
      if (MI.getOperand(1).getReg() != IC10::SP)
        return false;
      Operand = MI.getOperand(0);
      return true;
    }
    if (Opcode == IC10::STir) {
      if (MI.getOperand(1).getReg() != IC10::SP)
        return false;
      Operand = MI.getOperand(0);
      return true;
    }
    return false;
  }

  bool isStackLDOperand(const MachineInstr &MI, MachineOperand &Operand) {
    unsigned int Opcode = MI.getOpcode();
    if (Opcode != IC10::LDr)
      return false;
    if (MI.getOperand(1).getReg() != IC10::SP)
      return false;
    Operand = MI.getOperand(0);
    return true;
  }

  bool rewriteLDSequence(MachineBasicBlock::iterator Begin,
                         MachineBasicBlock::iterator End,
                         MachineBasicBlock &MBB,
                         SmallVector<OperandAndOffset, 4> &Items);
  bool rewriteSTSequence(MachineBasicBlock::iterator Begin,
                         MachineBasicBlock::iterator End,
                         MachineBasicBlock &MBB,
                         SmallVector<OperandAndOffset, 4> &Items);
  void writePushSetup(MachineBasicBlock::iterator MII, MachineBasicBlock &MBB,
                      const OperandAndOffset &Item);
  void writePush(MachineBasicBlock::iterator MII, MachineBasicBlock &MBB,
                 const OperandAndOffset &Item);
  void writePopSetup(MachineBasicBlock::iterator MII, MachineBasicBlock &MBB,
                     const OperandAndOffset &Item);
  void writePop(MachineBasicBlock::iterator MII, MachineBasicBlock &MBB,
                const OperandAndOffset &Item);
};

} // namespace

bool IC10LateCombinePushPopPassImpl::consume(MachineBasicBlock::iterator &MII,
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
  if (isStackLDOperand(SecondMI, Operand)) {
    RegionEnd = ++MII;
    Items.push_back({Operand, Offset, SecondMI.getDebugLoc()});
    while (MII != End) {
      MachineInstr &MIA = *MII++;
      if (MII == End || !isAddR15ToSP(MIA, Offset))
        break;
      MachineInstr &MIB = *MII;
      if (!isStackLDOperand(MIB, Operand))
        break;
      Items.push_back({Operand, Offset, MIB.getDebugLoc()});
      RegionEnd = ++MII;
    }
    return rewriteLDSequence(RegionBegin, RegionEnd, MBB, Items);
  }
  if (isStackSTOperand(SecondMI, Operand)) {
    RegionEnd = ++MII;
    Items.push_back({Operand, Offset, SecondMI.getDebugLoc()});
    while (MII != End) {
      MachineInstr &MIA = *MII++;
      if (MII == End || !isAddR15ToSP(MIA, Offset))
        break;
      MachineInstr &MIB = *MII;
      if (!isStackSTOperand(MIB, Operand))
        break;
      Items.push_back({Operand, Offset, MIB.getDebugLoc()});
      RegionEnd = ++MII;
    }
    return rewriteSTSequence(RegionBegin, RegionEnd, MBB, Items);
  }
  return false;
}

bool IC10LateCombinePushPopPassImpl::rewriteLDSequence(
    MachineBasicBlock::iterator Begin, MachineBasicBlock::iterator End,
    MachineBasicBlock &MBB, SmallVector<OperandAndOffset, 4> &Items) {
  sort(Items, OperandAndOffset::decending);
  // rewrite
  MBB.erase(Begin, End);
  int64_t CurrentSPOffset = INT64_MAX;
  for (auto &Item : Items) {
    if (CurrentSPOffset != Item.Offset + 1) {
      writePopSetup(End, MBB, Item);
      CurrentSPOffset = Item.Offset + 1;
    }
    writePop(End, MBB, Item);
    CurrentSPOffset--;
  }
  return true;
}

bool IC10LateCombinePushPopPassImpl::rewriteSTSequence(
    MachineBasicBlock::iterator Begin, MachineBasicBlock::iterator End,
    MachineBasicBlock &MBB, SmallVector<OperandAndOffset, 4> &Items) {
  if (Items.size() <= 1)
    return false;
  sort(Items, OperandAndOffset::ascending);
  // rewrite
  MBB.erase(Begin, End);
  int64_t CurrentSPOffset = INT64_MAX;
  for (auto &Item : Items) {
    if (CurrentSPOffset != Item.Offset) {
      writePushSetup(End, MBB, Item);
      CurrentSPOffset = Item.Offset;
    }
    writePush(End, MBB, Item);
    CurrentSPOffset++;
  }
  return true;
}

void IC10LateCombinePushPopPassImpl::writePushSetup(
    MachineBasicBlock::iterator MII, MachineBasicBlock &MBB,
    const OperandAndOffset &Item) {
  BuildMI(MBB, MII, Item.DL, TII.get(IC10::ADDri), IC10::SP)
      .addReg(IC10::R15)
      .addFPImm(ConstantFP::get(DoubleTy, Item.Offset));
}

void IC10LateCombinePushPopPassImpl::writePush(MachineBasicBlock::iterator MII,
                                               MachineBasicBlock &MBB,
                                               const OperandAndOffset &Item) {
  unsigned int Opcode = Item.Operand.isFPImm() ? IC10::PUSHi : IC10::PUSHr;
  BuildMI(MBB, MII, Item.DL, TII.get(Opcode)).add(Item.Operand);
}

void IC10LateCombinePushPopPassImpl::writePopSetup(
    MachineBasicBlock::iterator MII, MachineBasicBlock &MBB,
    const OperandAndOffset &Item) {
  BuildMI(MBB, MII, Item.DL, TII.get(IC10::ADDri), IC10::SP)
      .addReg(IC10::R15)
      .addFPImm(ConstantFP::get(DoubleTy, Item.Offset + 1));
}

void IC10LateCombinePushPopPassImpl::writePop(MachineBasicBlock::iterator MII,
                                              MachineBasicBlock &MBB,
                                              const OperandAndOffset &Item) {
  BuildMI(MBB, MII, Item.DL, TII.get(IC10::POP)).add(Item.Operand);
}

PreservedAnalyses IC10LateCombinePushPopPassImpl::apply(MachineFunction &MF) {
  LLVMContext &Context = MF.getFunction().getContext();
  DoubleTy = Type::getDoubleTy(Context);

  bool Changed = false;
  for (MachineBasicBlock &MBB : MF) {
    auto MII = MBB.begin();
    while (MII != MBB.end()) {
      Changed |= consume(MII, MBB.end(), MBB);
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
