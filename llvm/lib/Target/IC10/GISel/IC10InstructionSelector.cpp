// IC10

#include "IC10InstrInfo.h"
#include "IC10RegisterBankInfo.h"
#include "IC10RegisterInfo.h"
#include "IC10Subtarget.h"
#include "IC10TargetMachine.h"
#include "MCTargetDesc/IC10MCTargetDesc.h"
#include "llvm/CodeGen/GlobalISel/GIMatchTableExecutorImpl.h"
#include "llvm/CodeGen/GlobalISel/GenericMachineInstrs.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IntrinsicsIC10.h"
#include "llvm/Support/CodeGenCoverage.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include <assert.h>
#include <cassert>
#include <cstdint>
#include <optional>
#include <utility>

#define DEBUG_TYPE "ic10-isel"

using namespace llvm;

#define GET_GLOBALISEL_PREDICATE_BITSET
#include "IC10GenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATE_BITSET

namespace {

class IC10InstructionSelector : public InstructionSelector {
public:
  IC10InstructionSelector(const IC10TargetMachine &TM, const IC10Subtarget &STI,
                          const IC10RegisterBankInfo &RBI);

  bool select(MachineInstr &I) override;

  void setupMF(MachineFunction &MF, GISelValueTracking *VT,
               CodeGenCoverage *CoverageInfo, ProfileSummaryInfo *PSI,
               BlockFrequencyInfo *BFI) override {
    InstructionSelector::setupMF(MF, VT, CoverageInfo, PSI, BFI);
    MRI = &MF.getRegInfo();
  }

  static const char *getName() { return DEBUG_TYPE; }

private:
  bool selectImpl(MachineInstr &I, CodeGenCoverage &CoverageInfo) const;
  bool selectCondBR(MachineInstr &I) const;
  bool selectCondFCMPBR(MachineOperand Dest, MachineInstr &MI) const;
  bool selectFCMP(MachineInstr &I) const;
  bool selectICMP(MachineInstr &I) const;
  bool selectAssertedFCMP(MachineInstr &I) const;
  bool selectBranch(MachineInstr &I) const;

  void getOperandsForICMP(const MachineInstr &MI, IC10CC::CondCode &CC,
                          MachineOperand &LHS, MachineOperand &RHS,
                          bool &RHSIsImm) const;
  bool getOperandsForAssertedFCMP(const MachineInstr &MI, IC10CC::CondCode &CC,
                                  MachineOperand &LHS, MachineOperand &RHS,
                                  bool &RHSIsImm) const;

  // Select a reg + imm pattern for address.
  ComplexRendererFns selectAddrRegImm(MachineOperand &Root) const;

  const IC10TargetMachine &TM;
  const IC10InstrInfo &TII;
  const IC10RegisterInfo &TRI;
  const IC10RegisterBankInfo &RBI;

  MachineRegisterInfo *MRI = nullptr;

#define GET_GLOBALISEL_PREDICATES_DECL
#include "IC10GenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATES_DECL

#define GET_GLOBALISEL_TEMPORARIES_DECL
#include "IC10GenGlobalISel.inc"
#undef GET_GLOBALISEL_TEMPORARIES_DECL
};

} // end namespace

#define GET_GLOBALISEL_IMPL
#include "IC10GenGlobalISel.inc"
#undef GET_GLOBALISEL_IMPL

IC10InstructionSelector::IC10InstructionSelector(
    const IC10TargetMachine &TM, const IC10Subtarget &STI,
    const IC10RegisterBankInfo &RBI)
    : InstructionSelector(), TM(TM), TII(*STI.getInstrInfo()),
      TRI(*STI.getRegisterInfo()), RBI(RBI),

#define GET_GLOBALISEL_PREDICATES_INIT
#include "IC10GenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATES_INIT
#define GET_GLOBALISEL_TEMPORARIES_INIT
#include "IC10GenGlobalISel.inc"
#undef GET_GLOBALISEL_TEMPORARIES_INIT
{
}

bool IC10InstructionSelector::select(MachineInstr &I) {
  // Certain non-generic instructions also need some special handling.
  if (!isPreISelGenericOpcode(I.getOpcode()) && !I.isPreISelOpcode())
    return true;

  if (selectImpl(I, *CoverageInfo))
    return true;

  // Fallback for special instructions
  // MachineBasicBlock &MBB = *I.getParent();
  // MachineFunction &MF = *MBB.getParent();
  // MachineInstr *MI = nullptr;
  using namespace TargetOpcode;

  switch (I.getOpcode()) {
  case G_PHI:
  case PHI: {
    const Register DefReg = I.getOperand(0).getReg();
    const LLT DefTy = MRI->getType(DefReg);
    const RegClassOrRegBank &RegClassOrBank = MRI->getRegClassOrRegBank(DefReg);

    const TargetRegisterClass *DefRC =
        dyn_cast<const TargetRegisterClass *>(RegClassOrBank);
    // if (!DefRC) {
    //   if (!DefTy.isValid()) {
    //     LLVM_DEBUG(dbgs() << "PHI operand has no type, not a gvreg?\n");
    //     return false;
    //   }
    //   const RegisterBank &RB = *cast<const RegisterBank *>(RegClassOrBank);
    //   DefRC = getRegClassForTypeOnBank(DefTy, RB);
    //   if (!DefRC) {
    //     LLVM_DEBUG(dbgs() << "PHI operand has unexpected size/bank\n");
    //     return false;
    //   }
    // }

    I.setDesc(TII.get(TargetOpcode::PHI));

    return RBI.constrainGenericRegister(DefReg, *DefRC, *MRI);
  }
  case G_BRCOND:
    return selectCondBR(I);
  case G_BR:
    return selectBranch(I);
  case G_ICMP:
    return selectICMP(I);
  case G_FCMP:
    return selectFCMP(I);
  case IC10::G_FEQ:
  case IC10::G_FNE:
  case IC10::G_FGE:
  case IC10::G_FGT:
  case IC10::G_FLE:
  case IC10::G_FLT:
    return selectAssertedFCMP(I);
  default:
    return false;
  }
}

void IC10InstructionSelector::getOperandsForICMP(const MachineInstr &MI,
                                                 IC10CC::CondCode &CC,
                                                 MachineOperand &LHS,
                                                 MachineOperand &RHS,
                                                 bool &RHSIsImm) const {
  assert(MI.getOpcode() == TargetOpcode::G_ICMP);

  auto &CmpMI = cast<GICmp>(MI);
  CmpInst::Predicate Pred = CmpMI.getCond();

  switch (Pred) {
  default:
    llvm_unreachable("Unsupported predicate in ICMP.");
  case CmpInst::ICMP_EQ:
    CC = IC10CC::COND_IEQ;
    break;
  case CmpInst::ICMP_NE:
    CC = IC10CC::COND_INE;
    break;
  case CmpInst::ICMP_UGT:
  case CmpInst::ICMP_SGT:
    CC = IC10CC::COND_IGT;
    break;
  case CmpInst::ICMP_UGE:
  case CmpInst::ICMP_SGE:
    CC = IC10CC::COND_IGE;
    break;
  case CmpInst::ICMP_ULT:
  case CmpInst::ICMP_SLT:
    CC = IC10CC::COND_ILT;
    break;
  case CmpInst::ICMP_SLE:
  case CmpInst::ICMP_ULE:
    CC = IC10CC::COND_ILE;
    break;
  }

  auto C1 = getIConstantVRegSExtVal(LHS.getReg(), *MRI);
  auto C2 = getIConstantVRegSExtVal(RHS.getReg(), *MRI);

  RHSIsImm = false;
  if (C2.has_value()) {
    RHSIsImm = true;
    RHS.ChangeToImmediate(C2.value());
  } else if (C1.has_value()) {
    std::swap(LHS, RHS);
    CC = IC10CC::flipped(CC);
    RHSIsImm = true;
    RHS.ChangeToImmediate(C1.value());
  }
}

bool IC10InstructionSelector::getOperandsForAssertedFCMP(const MachineInstr &MI,
                                                         IC10CC::CondCode &CC,
                                                         MachineOperand &LHS,
                                                         MachineOperand &RHS,
                                                         bool &RHSIsImm) const {
  switch (MI.getOpcode()) {
  default:
    return false;
  case IC10::G_FEQ:
    CC = IC10CC::COND_FEQ;
    break;
  case IC10::G_FNE:
    CC = IC10CC::COND_FNE;
    break;
  case IC10::G_FGT:
    CC = IC10CC::COND_FGT;
    break;
  case IC10::G_FGE:
    CC = IC10CC::COND_FGE;
    break;
  case IC10::G_FLT:
    CC = IC10CC::COND_FLT;
    break;
  case IC10::G_FLE:
    CC = IC10CC::COND_FLE;
    break;
  }

  auto *C1 = getConstantFPVRegVal(LHS.getReg(), *MRI);
  auto *C2 = getConstantFPVRegVal(RHS.getReg(), *MRI);

  RHSIsImm = false;
  if (C2) {
    RHSIsImm = true;
    RHS.ChangeToFPImmediate(C2);
  } else if (C1) {
    std::swap(LHS, RHS);
    CC = IC10CC::flipped(CC);
    RHSIsImm = true;
    RHS.ChangeToFPImmediate(C1);
  }
  return true;
}

bool IC10InstructionSelector::selectICMP(MachineInstr &MI) const {
  assert(MI.getOpcode() == TargetOpcode::G_ICMP);
  MachineBasicBlock &MBB = *MI.getParent();

  Register Dest = MI.getOperand(0).getReg();
  MRI->setRegClass(Dest, &IC10::GPRRegClass);

  MachineOperand LHS = MI.getOperand(1);
  MachineOperand RHS = MI.getOperand(2);
  IC10CC::CondCode CC = IC10CC::COND_INVALID;
  bool RHSIsImm = false;
  getOperandsForICMP(MI, CC, LHS, RHS, RHSIsImm);
  assert(CC != IC10CC::COND_INVALID);

  unsigned NewOpcode = TII.getCmpOpcode(CC, RHSIsImm);
  BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(NewOpcode), Dest)
      .add(LHS)
      .add(RHS);

  MI.removeFromParent();
  return true;
}

bool IC10InstructionSelector::selectFCMP(MachineInstr &MI) const {
  assert(MI.getOpcode() == TargetOpcode::G_FCMP);
  MachineBasicBlock &MBB = *MI.getParent();
  DebugLoc DL = MI.getDebugLoc();

  Register Dest = MI.getOperand(0).getReg();
  MRI->setRegClass(Dest, &IC10::GPRRegClass);

  MachineOperand LHS = MI.getOperand(2);
  MachineOperand RHS = MI.getOperand(3);

  auto &CmpMI = cast<GFCmp>(MI);
  CmpInst::Predicate Pred = CmpMI.getCond();

  assert(Pred == CmpInst::FCMP_ORD || Pred == CmpInst::FCMP_UNO);
  bool OrderedAnd = Pred == CmpInst::FCMP_ORD;

  const ConstantFP *C1 = getConstantFPVRegVal(LHS.getReg(), *MRI);
  const ConstantFP *C2 = getConstantFPVRegVal(RHS.getReg(), *MRI);
  bool IsC1NAN = C1 && isnan(C1->getValue().convertToDouble());
  bool IsC2NAN = C2 && isnan(C2->getValue().convertToDouble());

  // place non-nan constant at C2
  if (C1 && !IsC1NAN) {
    std::swap(LHS, RHS);
    std::swap(C1, C2);
    std::swap(IsC1NAN, IsC2NAN);
  }
  bool RhsIsNonNANImm = C2 && !IsC2NAN;

  // select instructions for nan comparisons
  unsigned int CmpOpcode = OrderedAnd ? IC10::SNANZr : IC10::SNANr;
  // single argument nan comparison
  if (RhsIsNonNANImm) {
    BuildMI(MBB, MI, DL, TII.get(CmpOpcode), Dest).add(LHS);
    MI.removeFromParent();
    return true;
  }
  // multi-argument nan comparison
  Register NANCheckLHSReg = MRI->createVirtualRegister(&IC10::GPRRegClass);
  Register NANCheckRHSReg = MRI->createVirtualRegister(&IC10::GPRRegClass);
  BuildMI(MBB, MI, DL, TII.get(CmpOpcode), NANCheckLHSReg).add(LHS);
  BuildMI(MBB, MI, DL, TII.get(CmpOpcode), NANCheckRHSReg).add(RHS);
  unsigned int JoinOpcode = OrderedAnd ? IC10::ANDrr : IC10::ORrr;
  BuildMI(MBB, MI, DL, TII.get(JoinOpcode), Dest)
      .addReg(NANCheckLHSReg)
      .addReg(NANCheckRHSReg);
  MI.removeFromParent();
  return true;
}

bool IC10InstructionSelector::selectCondFCMPBR(MachineOperand Target,
                                               MachineInstr &MI) const {
  assert(MI.getOpcode() == TargetOpcode::G_FCMP);
  MachineBasicBlock &MBB = *MI.getParent();
  DebugLoc DL = MI.getDebugLoc();

  MachineOperand LHS = MI.getOperand(2);
  MachineOperand RHS = MI.getOperand(3);

  auto &CmpMI = cast<GFCmp>(MI);
  CmpInst::Predicate Pred = CmpMI.getCond();

  assert(Pred == CmpInst::FCMP_ORD || Pred == CmpInst::FCMP_UNO);
  if (Pred == CmpInst::FCMP_ORD)
    return false;

  const ConstantFP *C1 = getConstantFPVRegVal(LHS.getReg(), *MRI);
  bool IsC1NAN = C1 && isnan(C1->getValue().convertToDouble());
  if (C1 && !IsC1NAN) {
    BuildMI(MBB, MI, DL, TII.get(IC10::BNANr)).add(RHS).add(Target);
    MI.removeFromParent();
    return true;
  }

  const ConstantFP *C2 = getConstantFPVRegVal(RHS.getReg(), *MRI);
  bool IsC2NAN = C2 && isnan(C2->getValue().convertToDouble());
  if (C2 && !IsC2NAN) {
    BuildMI(MBB, MI, DL, TII.get(IC10::BNANr)).add(LHS).add(Target);
    MI.removeFromParent();
    return true;
  }
  return false;
}

bool IC10InstructionSelector::selectAssertedFCMP(MachineInstr &MI) const {
  MachineBasicBlock &MBB = *MI.getParent();

  IC10CC::CondCode CC = IC10CC::COND_INVALID;
  Register Dest = MI.getOperand(0).getReg();
  MRI->setRegClass(Dest, &IC10::GPRRegClass);

  MachineOperand LHS = MI.getOperand(1);
  MachineOperand RHS = MI.getOperand(2);
  bool RHSIsImm = false;
  bool Success = getOperandsForAssertedFCMP(MI, CC, LHS, RHS, RHSIsImm);
  assert(Success && CC != IC10CC::COND_INVALID);

  unsigned Opcode = TII.getCmpOpcode(CC, RHSIsImm);
  BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(Opcode), Dest)
      .add(LHS)
      .add(RHS);
  MI.removeFromParent();
  return true;
}

bool IC10InstructionSelector::selectCondBR(MachineInstr &MI) const {
  assert(MI.getOpcode() == TargetOpcode::G_BRCOND);
  MachineBasicBlock &MBB = *MI.getParent();

  Register Cond = MI.getOperand(0).getReg();
  MachineInstr *Cmp = MRI->getVRegDef(Cond);
  if (Cmp && (Cmp->getOpcode() == TargetOpcode::G_ICMP)) {
    IC10CC::CondCode CC = IC10CC::COND_INVALID;
    MachineOperand LHS = Cmp->getOperand(2);
    MachineOperand RHS = Cmp->getOperand(3);
    bool RHSIsImm = false;
    getOperandsForICMP(*Cmp, CC, LHS, RHS, RHSIsImm);
    assert(CC != IC10CC::COND_INVALID);

    unsigned Opcode = TII.getBrCCOpcode(CC, RHSIsImm);
    BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(Opcode))
        .add(LHS)
        .add(RHS)
        .add(MI.getOperand(1));
    MI.removeFromParent();
    return true;
  }

  if (Cmp && (Cmp->getOpcode() == TargetOpcode::G_FCMP))
    if (selectCondFCMPBR(MI.getOperand(1), *Cmp))
      return true;

  MachineOperand LHS = Cmp->getOperand(1);
  MachineOperand RHS = Cmp->getOperand(2);
  IC10CC::CondCode CC = IC10CC::COND_INVALID;
  bool RHSIsImm = false;
  if (Cmp && getOperandsForAssertedFCMP(*Cmp, CC, LHS, RHS, RHSIsImm)) {
    unsigned Opcode = TII.getBrCCOpcode(CC, RHSIsImm);
    BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(Opcode))
        .add(LHS)
        .add(RHS)
        .add(MI.getOperand(1));
    MI.removeFromParent();
    return true;
  }

  // build regular boolean branch
  BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(IC10::BNEIri))
      .addReg(Cond)
      .addImm(0)
      .add(MI.getOperand(1));
  MI.removeFromParent();
  return true;
}

bool IC10InstructionSelector::selectBranch(MachineInstr &MI) const {
  assert(MI.getOpcode() == TargetOpcode::G_BR);
  MachineBasicBlock &MBB = *MI.getParent();
  DebugLoc DL = MI.getDebugLoc();
  BuildMI(MBB, MI, DL, TII.get(IC10::Ji)).addMBB(MI.getOperand(0).getMBB());
  MI.eraseFromParent();
  return true;
}

InstructionSelector::ComplexRendererFns
IC10InstructionSelector::selectAddrRegImm(MachineOperand &Root) const {
  if (!Root.isReg())
    return std::nullopt;

  MachineInstr *RootDef = MRI->getVRegDef(Root.getReg());
  if (RootDef->getOpcode() == TargetOpcode::G_FRAME_INDEX) {
    return {{
        [=](MachineInstrBuilder &MIB) { MIB.add(RootDef->getOperand(1)); },
        [=](MachineInstrBuilder &MIB) { MIB.addImm(0); },
    }};
  }

  if (isBaseWithConstantOffset(Root, *MRI)) {
    assert(RootDef->getOpcode() == IC10::G_PTR_ADD);

    Register BaseReg = RootDef->getOperand(1).getReg();
    Register OffsetReg = RootDef->getOperand(2).getReg();

    auto C = getIConstantVRegVal(OffsetReg, *MRI);
    int64_t Offset = C.value().getSExtValue();
    assert(Offset % 8 == 0 && "Alignment violated!");

    MachineInstr *BaseDef = MRI->getVRegDef(BaseReg);
    if (BaseDef->getOpcode() == TargetOpcode::G_FRAME_INDEX) {
      return {
          {[=](MachineInstrBuilder &MIB) { MIB.add(BaseDef->getOperand(1)); },
           [=](MachineInstrBuilder &MIB) { MIB.addImm(Offset / 8); }}};
    }
    return {{[=](MachineInstrBuilder &MIB) { MIB.addReg(BaseReg); },
             [=](MachineInstrBuilder &MIB) { MIB.addImm(Offset / 8); }}};
  }
  return {{[=](MachineInstrBuilder &MIB) { MIB.addReg(Root.getReg()); },
           [=](MachineInstrBuilder &MIB) { MIB.addImm(0); }}};
}

namespace llvm {
InstructionSelector *
createIC10InstructionSelector(const IC10TargetMachine &TM,
                              const IC10Subtarget &Subtarget,
                              const IC10RegisterBankInfo &RBI) {
  return new IC10InstructionSelector(TM, Subtarget, RBI);
}
} // end namespace llvm
