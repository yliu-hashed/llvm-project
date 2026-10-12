// IC10
#include "IC10InstrInfo.h"
#include "IC10.h"
#include "IC10MachineFunctionInfo.h"
#include "IC10RegisterInfo.h"
#include "IC10Subtarget.h"
#include "MCTargetDesc/IC10MCTargetDesc.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <complex.h>

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "IC10GenInstrInfo.inc"


IC10CC::CondCode IC10CC::flipped(IC10CC::CondCode CC) {
  switch (CC) {
  case COND_INVALID:
    return COND_INVALID;
  case COND_IEQ:
    return COND_IEQ;
  case COND_INE:
    return COND_INE;
  case COND_IGE:
    return COND_ILE;
  case COND_IGT:
    return COND_ILT;
  case COND_ILE:
    return COND_IGE;
  case COND_ILT:
    return COND_IGT;
    // float CC
  case COND_FEQ:
    return COND_FEQ;
  case COND_FNE:
    return COND_FNE;
  case COND_FGE:
    return COND_FLE;
  case COND_FGT:
    return COND_FLT;
  case COND_FLE:
    return COND_FGE;
  case COND_FLT:
    return COND_FGT;
  case COND_FNAN:
  case COND_FNANZ:
    llvm_unreachable("Cannot flip the arguments of single operand conditions.");
  }
}

IC10CC::CondCode IC10CC::negated(IC10CC::CondCode CC) {
  switch (CC) {
  case COND_INVALID:
    return COND_INVALID;
  case COND_IEQ:
    return COND_INE;
  case COND_INE:
    return COND_IEQ;
  case COND_IGE:
    return COND_ILT;
  case COND_IGT:
    return COND_ILE;
  case COND_ILE:
    return COND_IGT;
  case COND_ILT:
    return COND_IGE;
    // float CC
  case COND_FEQ:
    return COND_FNE;
  case COND_FNE:
    return COND_FEQ;
  case COND_FGE:
    return COND_FLT;
  case COND_FGT:
    return COND_FLE;
  case COND_FLE:
    return COND_FGT;
  case COND_FLT:
    return COND_FGE;
  case COND_FNAN:
    return COND_FNANZ;
  case COND_FNANZ:
    return COND_FNAN;
  }
}

// Pin the vtable to this file.
void IC10InstrInfo::anchor() {}

IC10InstrInfo::IC10InstrInfo(IC10Subtarget &STI) :
    IC10GenInstrInfo(STI, RI, IC10::ADJCALLSTACKDOWN, IC10::ADJCALLSTACKUP),
    RI(), Subtarget(STI) {}

unsigned int
IC10InstrInfo::getBrCCOpcode(IC10CC::CondCode CC, bool RHSIsImm) const {
  switch (CC) {
  case IC10CC::COND_INVALID:
    llvm_unreachable("Cannot get opcode for invalid condition");
    // integer CC
  case IC10CC::COND_IEQ:
    return RHSIsImm ? IC10::BEQIri : IC10::BEQIrr;
  case IC10CC::COND_INE:
    return RHSIsImm ? IC10::BNEIri : IC10::BNEIrr;
  case IC10CC::COND_IGE:
    return RHSIsImm ? IC10::BGEIri : IC10::BGEIrr;
  case IC10CC::COND_IGT:
    return RHSIsImm ? IC10::BGTIri : IC10::BGTIrr;
  case IC10CC::COND_ILE:
    return RHSIsImm ? IC10::BLEIri : IC10::BLEIrr;
  case IC10CC::COND_ILT:
    return RHSIsImm ? IC10::BLTIri : IC10::BLTIrr;
    // floating point CC
  case IC10CC::COND_FEQ:
    return RHSIsImm ? IC10::BEQri : IC10::BEQrr;
  case IC10CC::COND_FNE:
    return RHSIsImm ? IC10::BNEri : IC10::BNErr;
  case IC10CC::COND_FGE:
    return RHSIsImm ? IC10::BGEri : IC10::BGErr;
  case IC10CC::COND_FGT:
    return RHSIsImm ? IC10::BGTri : IC10::BGTrr;
  case IC10CC::COND_FLE:
    return RHSIsImm ? IC10::BLEri : IC10::BLErr;
  case IC10CC::COND_FLT:
    return RHSIsImm ? IC10::BLTri : IC10::BLTrr;
  case IC10CC::COND_FNAN:
    return IC10::BNANr;
  case IC10CC::COND_FNANZ:
    llvm_unreachable("IC10 does not have the branch not nan instruction.");
  }
}

unsigned int
IC10InstrInfo::getCmpOpcode(IC10CC::CondCode CC, bool RHSIsImm) const {
  switch (CC) {
  case IC10CC::COND_INVALID:
    llvm_unreachable("Cannot get opcode for invalid condition");
    // integer CC
  case IC10CC::COND_IEQ:
    return RHSIsImm ? IC10::SEQIri : IC10::SEQIrr;
  case IC10CC::COND_INE:
    return RHSIsImm ? IC10::SNEIri : IC10::SNEIrr;
  case IC10CC::COND_IGE:
    return RHSIsImm ? IC10::SGEIri : IC10::SGEIrr;
  case IC10CC::COND_IGT:
    return RHSIsImm ? IC10::SGTIri : IC10::SGTIrr;
  case IC10CC::COND_ILE:
    return RHSIsImm ? IC10::SLEIri : IC10::SLEIrr;
  case IC10CC::COND_ILT:
    return RHSIsImm ? IC10::SLTIri : IC10::SLTIrr;
    // floating point CC
  case IC10CC::COND_FEQ:
    return RHSIsImm ? IC10::SEQri : IC10::SEQrr;
  case IC10CC::COND_FNE:
    return RHSIsImm ? IC10::SNEri : IC10::SNErr;
  case IC10CC::COND_FGE:
    return RHSIsImm ? IC10::SGEri : IC10::SGErr;
  case IC10CC::COND_FGT:
    return RHSIsImm ? IC10::SGTri : IC10::SGTrr;
  case IC10CC::COND_FLE:
    return RHSIsImm ? IC10::SLEri : IC10::SLErr;
  case IC10CC::COND_FLT:
    return RHSIsImm ? IC10::SLTri : IC10::SLTrr;
  case IC10CC::COND_FNAN:
    return IC10::SNANr;
  case IC10CC::COND_FNANZ:
    return IC10::SNANZr;
  }
}

IC10CC::CondCode
IC10InstrInfo::getBrOpcodeCC(unsigned int Opcode) const {
  switch (Opcode) {
  default:
    return IC10CC::COND_INVALID;
    // floating point CC
  case IC10::BEQIri:
  case IC10::BEQIrr:
    return IC10CC::COND_IEQ;
  case IC10::BNEIri:
  case IC10::BNEIrr:
    return IC10CC::COND_INE;
  case IC10::BGEIri:
  case IC10::BGEIrr:
    return IC10CC::COND_IGE;
  case IC10::BGTIri:
  case IC10::BGTIrr:
    return IC10CC::COND_IGT;
  case IC10::BLEIri:
  case IC10::BLEIrr:
    return IC10CC::COND_ILE;
  case IC10::BLTIri:
  case IC10::BLTIrr:
    return IC10CC::COND_ILT;
    // floating point CC
  case IC10::BEQri:
  case IC10::BEQrr:
    return IC10CC::COND_FEQ;
  case IC10::BNEri:
  case IC10::BNErr:
    return IC10CC::COND_FNE;
  case IC10::BGEri:
  case IC10::BGErr:
    return IC10CC::COND_FGE;
  case IC10::BGTri:
  case IC10::BGTrr:
    return IC10CC::COND_FGT;
  case IC10::BLEri:
  case IC10::BLErr:
    return IC10CC::COND_FLE;
  case IC10::BLTri:
  case IC10::BLTrr:
    return IC10CC::COND_FLT;
  case IC10::BNANr:
    return IC10CC::COND_FNAN;
  }
}

void IC10InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MI, const DebugLoc &DL,
                                Register DestReg, Register SrcReg, bool KillSrc,
                                bool RenamableDest,
                                bool RenamableSrc) const {
  if (!IC10::GPRRegClass.contains(DestReg, SrcReg)) {
    llvm_unreachable("impossible copy");
  }
  const bool IsInteger = IC10::GPRRegClass.contains(DestReg);
  if (IsInteger) {
    BuildMI(MBB, MI, DL, get(IC10::IMOVEr), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
  } else {
    BuildMI(MBB, MI, DL, get(IC10::MOVEr), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
  }
}

void IC10InstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MI,
                                        Register SrcReg, bool isKill, int FI,
                                        const TargetRegisterClass *RC,
                                        Register VReg,
                                        MachineInstr::MIFlag Flags) const {
  // put frame index in place of the frame register
  const bool IsInteger = IC10::GPRRegClass.hasSubClassEq(RC);
  BuildMI(MBB, MI, DebugLoc(), get(IC10::IADDri), IC10::SP)
      .addFrameIndex(FI)
      .addImm(0);
  if (IsInteger) {
    BuildMI(MBB, MI, DebugLoc(), get(IC10::STrr))
        .addReg(SrcReg, getKillRegState(isKill))
        .addReg(IC10::SP);
  } else {
    BuildMI(MBB, MI, DebugLoc(), get(IC10::STrr))
        .addReg(SrcReg, getKillRegState(isKill))
        .addReg(IC10::SP);
  }
}

void IC10InstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                         MachineBasicBlock::iterator MI,
                                         Register DestReg, int FI,
                                         const TargetRegisterClass *RC,
                                         Register VReg, unsigned SubReg,
                                         MachineInstr::MIFlag Flags) const {
  // put frame index in place of the frame register
  const bool IsInteger = IC10::GPRRegClass.hasSubClassEq(RC);
  BuildMI(MBB, MI, DebugLoc(), get(IC10::IADDri), IC10::SP)
      .addFrameIndex(FI)
      .addImm(0);
  if (IsInteger) {
    BuildMI(MBB, MI, DebugLoc(), get(IC10::ILDr), DestReg)
        .addReg(IC10::SP);
  } else {
    BuildMI(MBB, MI, DebugLoc(), get(IC10::LDr), DestReg)
        .addReg(IC10::SP);
  }
}

//===----------------------------------------------------------------------===//
// Branch Analyze
//===----------------------------------------------------------------------===//

bool IC10InstrInfo::isUnpredicatedTerminator(const MachineInstr &MI) const {
  if (!MI.isTerminator())
    return false;
  // Conditional branch is a special case.
  if (MI.isBranch() && !MI.isBarrier())
    return true;
  if (!MI.isPredicable())
    return true;
  return !isPredicated(MI);
}

static int countAndErase(MachineBasicBlock::iterator I,
                         MachineBasicBlock::iterator End) {
  int Count = 0;
  while (I != End) {
    I->eraseFromParent();
    Count++;
  }
  return Count;
}

/// Analyze the branching code at the end of MBB, returning
/// true if it cannot be understood (e.g. it's a switch dispatch or isn't
/// implemented for a target).  Upon success, this returns false and returns
/// with the following information in various cases:
///
/// 1. If this block ends with no branches (it just falls through to its succ)
///    just return false, leaving TBB/FBB null.
/// 2. If this block ends with only an unconditional branch, it sets TBB to be
///    the destination block.
/// 3. If this block ends with a conditional branch and it falls through to a
///    successor block, it sets TBB to be the branch destination block and a
///    list of operands that evaluate the condition. These operands can be
///    passed to other TargetInstrInfo methods to create new branches.
/// 4. If this block ends with a conditional branch followed by an
///    unconditional branch, it returns the 'true' destination in TBB, the
///    'false' destination in FBB, and a list of operands that evaluate the
///    condition.  These operands can be passed to other TargetInstrInfo
///    methods to create new branches.
///
/// Note that removeBranch and insertBranch must be implemented to support
/// cases where this method returns success.
///
/// If AllowModify is true, then this routine is allowed to modify the basic
/// block (e.g. delete instructions after the unconditional branch).
///
/// The CFG information in MBB.Predecessors and MBB.Successors must be valid
/// before calling this function.
bool IC10InstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                  MachineBasicBlock *&TBB,
                                  MachineBasicBlock *&FBB,
                                  SmallVectorImpl<MachineOperand> &Cond,
                                  bool AllowModify) const {
  TBB = FBB = nullptr;
  Cond.clear();
  MachineBasicBlock::iterator I = MBB.getFirstNonDebugInstr(false);
  // find first branching instr
  for (; I != MBB.end(); I++) {
    if (I->isBranch() || I->isBarrier() || I->isTerminator()) break;
  }
  // fallthrough or empty
  if (I == MBB.end())
    return false;
  // return and barrier
  if (I->getOpcode() == IC10::Ji) {
    TBB = I->getOperand(0).getMBB();
    if (AllowModify)
      MBB.erase(++I, MBB.end());
    return false;
  }
  if (I->isBarrier()) {
    if (AllowModify)
      MBB.erase(++I, MBB.end());
    return true;
  }
  if (!I->isConditionalBranch()) // no idea what to do if it's not a conditional branch
    return true;

  // get branch out
  IC10CC::CondCode CC = getBrOpcodeCC(I->getOpcode());
  assert(CC != IC10CC::COND_INVALID);
  Cond.emplace_back(MachineOperand::CreateImm(CC));
  Cond.emplace_back(I->getOperand(0)); // LHS
  if (IC10CC::isSingle(CC)) {
    TBB = I->getOperand(1).getMBB();
  } else {
    Cond.emplace_back(I->getOperand(1)); // RHS
    TBB = I->getOperand(2).getMBB();
  }

  // get false condition
  I++;
  if (I == MBB.end()) // fallthrough on the false condition
    return false;
  assert(I->getOpcode() == IC10::Ji);
  FBB = I->getOperand(0).getMBB();

  // cleanup whats rest
  if (AllowModify)
    MBB.erase(++I, MBB.end());

  return false;
}

unsigned IC10InstrInfo::insertBranch(MachineBasicBlock &MBB,
                                     MachineBasicBlock *TBB,
                                     MachineBasicBlock *FBB,
                                     ArrayRef<MachineOperand> Cond,
                                     const DebugLoc &DL,
                                     int *BytesAdded) const {
  assert(TBB && "insertBranch must not be told to insert a fallthrough");
  int BytesAddedTmp = 8;

  // unconditional branch
  if (Cond.empty()) {
    assert(FBB == nullptr);
    BuildMI(&MBB, DL, get(IC10::Ji))
        .addMBB(TBB);
    if (BytesAdded)
      *BytesAdded = BytesAddedTmp;
    return 1;
  }

  // conditional branch
  assert(Cond.size() > 0);
  IC10CC::CondCode CC = (IC10CC::CondCode)Cond[0].getImm();
  assert(CC != IC10CC::COND_FNANZ);
  if (CC == IC10CC::COND_FNAN) {
    BuildMI(&MBB, DL, get(IC10::BNANr))
        .add(Cond[1])
        .addMBB(TBB);
  } else {
    MachineOperand RHS = Cond[2];
    bool RHSIsImm = RHS.isImm() || RHS.isFPImm();
    unsigned int Opcode = getBrCCOpcode(CC, RHSIsImm);
    BuildMI(&MBB, DL, get(Opcode))
        .add(Cond[1])
        .add(Cond[2])
        .addMBB(TBB);
  }

  if (!FBB) {
    if (BytesAdded)
      *BytesAdded = BytesAddedTmp;
    return 1;
  }

  BuildMI(&MBB, DL, get(IC10::Ji))
      .addMBB(FBB);
  BytesAddedTmp += 8;

  if (BytesAdded)
    *BytesAdded = BytesAddedTmp;
  return 2;
}

unsigned IC10InstrInfo::removeBranch(MachineBasicBlock &MBB,
                                     int *BytesRemoved) const {
  MachineBasicBlock::iterator I = MBB.getFirstNonDebugInstr(false);
  // find first branching instr
  for (; I != MBB.end(); I++) {
    if (I->isBranch() || I->isBarrier() || I->isTerminator()) break;
  }
  // fallthrough or empty
  assert(I != MBB.end());
  // return and barrier
  if (I->getOpcode() == IC10::Ji) {
    (I++)->eraseFromParent();
    int Count = countAndErase(I, MBB.end());
    if (BytesRemoved)
      *BytesRemoved = 8 + Count * 8;
    return 1;
  }
  assert(I->isConditionalBranch());
  (I++)->eraseFromParent();

  // get false condition
  if (I == MBB.end()) { // fallthrough on the false condition
    if (BytesRemoved)
      *BytesRemoved = 8;
    return 1;
  }
  assert(I->getOpcode() == IC10::Ji);
  (I++)->eraseFromParent();
  int Count = countAndErase(I, MBB.end());
  if (BytesRemoved)
    *BytesRemoved = 16 + Count * 8;
  return 2;
}

bool IC10InstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  if (Cond.size() != 3)
    return false;
  IC10CC::CondCode CC = (IC10CC::CondCode)Cond[0].getImm();
  Cond[0] = MachineOperand::CreateImm(IC10CC::negated(CC));
  return true;
}

MachineBasicBlock *
IC10InstrInfo::getBranchDestBlock(const MachineInstr &MI) const {
  assert(MI.getDesc().isBranch() && "Unexpected opcode!");
  // The branch target is always the last operand.
  int NumOp = MI.getNumExplicitOperands();
  return MI.getOperand(NumOp - 1).getMBB();
}
