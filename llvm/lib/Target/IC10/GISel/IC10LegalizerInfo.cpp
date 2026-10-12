// IC10

#include "IC10LegalizerInfo.h"
#include "IC10InstrInfo.h"
#include "IC10RegisterInfo.h"
#include "MCTargetDesc/IC10MCTargetDesc.h"
#include "llvm/CodeGen/GlobalISel/GISelChangeObserver.h"
#include "llvm/CodeGen/GlobalISel/GenericMachineInstrs.h"
#include "llvm/CodeGen/GlobalISel/LegalizerHelper.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/RegisterPressure.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/CodeGenTypes/LowLevelType.h"
#include "llvm/IR/FMF.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <cmath>
#include <cstdint>

using namespace llvm;

IC10LegalizerInfo::IC10LegalizerInfo(const IC10Subtarget &ST) {
  using namespace TargetOpcode;
  // scalars
  const LLT s8 = LLT::scalar(8);
  const LLT s16 = LLT::scalar(16);
  const LLT s32 = LLT::scalar(32);
  const LLT s64 = LLT::scalar(64);
  // integers
  const LLT i32 = LLT::integer(32);
  const LLT i64 = LLT::integer(64);
  // floats
  const LLT f32 = LLT::float32();
  const LLT f64 = LLT::float64();
  // pointers
  const LLT p0 = LLT::pointer(0, 64);

  getActionDefinitionsBuilder(G_PHI)
      .legalFor({p0, i64, f64})
      .clampScalar(0, s64, s64);
  getActionDefinitionsBuilder(G_SELECT)
      .legalForCartesianProduct({f64, i64, p0}, {i64})
      .clampScalar(0, s64, s64)
      .clampScalar(1, s64, s64);


  getActionDefinitionsBuilder({G_INTRINSIC, G_INTRINSIC_W_SIDE_EFFECTS, G_TRAP})
      .alwaysLegal();

  getActionDefinitionsBuilder({G_SCMP, G_UCMP}).lower();

  getActionDefinitionsBuilder(G_BR).alwaysLegal();

  getActionDefinitionsBuilder(G_BRCOND).legalFor({i64}).clampScalar(0, s64,
                                                                    s64);

  getActionDefinitionsBuilder(G_ICMP)
      .legalFor({{i64, i64}, {p0, p0}})
      .clampScalar(0, s64, s64)
      .clampScalar(1, s64, s64);

  getActionDefinitionsBuilder({G_MEMSET, G_MEMCPY}).lower();

  getActionDefinitionsBuilder({G_ADD, G_SUB, G_AND, G_OR, G_XOR})
      .legalFor({i64})
      .clampScalar(0, s64, s64);

  getActionDefinitionsBuilder(G_MUL).legalFor({i64}).clampScalar(0, s64, s64);

  getActionDefinitionsBuilder({G_SHL, G_ASHR, G_LSHR})
      .legalFor({{i64, i64}})
      .clampScalar(0, s64, s64)
      .clampScalar(1, s64, s64);

  getActionDefinitionsBuilder(G_CONSTANT)
      .legalFor({i64, p0})
      .clampScalar(0, s64, s64);

  getActionDefinitionsBuilder({G_FRAME_INDEX, G_GLOBAL_VALUE}).legalFor({p0});

  getActionDefinitionsBuilder({G_SEXTLOAD, G_ZEXTLOAD}).custom();

  getActionDefinitionsBuilder({G_LOAD, G_STORE})
      .legalForTypesWithMemDesc(
          {{i64, p0, i64, 8}, {f64, p0, f64, 8}, {p0, p0, s64, 8}})
      .customFor({s8, s16, s32});

  getActionDefinitionsBuilder(G_PTR_ADD).legalFor({{p0, i64}});

  getActionDefinitionsBuilder({G_TRUNC, G_ANYEXT}).alwaysLegal();

  getActionDefinitionsBuilder(G_SEXT_INREG).lower();

  getActionDefinitionsBuilder({G_FPTOSI, G_FPTOUI})
      .legalFor({{i64, f64}})
      .clampScalar(0, s64, s64)
      .clampScalar(1, s64, s64);

  getActionDefinitionsBuilder({G_SITOFP, G_UITOFP})
      .legalFor({{f64, i64}})
      .clampScalar(0, s64, s64)
      .clampScalar(1, s64, s64);

  getActionDefinitionsBuilder(G_FCMP)
      .clampScalar(0, s64, s64)
      .clampScalar(1, s64, s64)
      .customFor({{s64, s64}});

  getActionDefinitionsBuilder(G_IS_FPCLASS)
      .legalFor({{i64, f64}})
      .clampScalar(0, s64, s64)
      .clampScalar(1, s64, s64);

  getActionDefinitionsBuilder({G_FADD, G_FSUB, G_FMUL, G_FDIV, G_FPOW,
                               G_FMAXIMUM, G_FMINIMUM, G_FATAN2})
      .legalFor({f64})
      .clampScalar(0, s64, s64);

  getActionDefinitionsBuilder({G_FLOG10, G_FLOG2, G_FEXP, G_FEXP10, G_FEXP2})
      .lower();

  getActionDefinitionsBuilder({G_FABS, G_INTRINSIC_TRUNC, G_INTRINSIC_ROUND,
                               G_FFLOOR, G_FCEIL, G_FSIN, G_FCOS, G_FTAN,
                               G_FASIN, G_FACOS, G_FATAN, G_FSQRT, G_FLOG})
      .legalFor({f64})
      .clampScalar(0, s64, s64);

  getActionDefinitionsBuilder(G_FCONSTANT)
      .legalFor({f64})
      .clampScalar(0, s64, s64);
}

namespace {

bool legalizeLoad(MachineInstr &MI, MachineRegisterInfo &MRI,
                  MachineIRBuilder &MIRBuilder, GISelChangeObserver &Observer) {
  MachineFunction *MF = MI.getMF();
  unsigned int Opcode = MI.getOpcode();
  assert(Opcode == TargetOpcode::G_LOAD || Opcode == TargetOpcode::G_SEXTLOAD ||
         Opcode == TargetOpcode::G_ZEXTLOAD);
  bool NeedSExt = Opcode == TargetOpcode::G_SEXTLOAD;
  bool NeedZExt = Opcode == TargetOpcode::G_ZEXTLOAD;

  Register Dest = MI.getOperand(0).getReg();
  Register Addr = MI.getOperand(1).getReg();

  LLT OrigTy = MRI.getType(Dest);
  assert(MI.memoperands().size() == 1);

  const LLT i64 = LLT::integer(64);

  MachineMemOperand *OldMMO = *MI.memoperands_begin();
  MachineMemOperand *NewMMO = MF->getMachineMemOperand(OldMMO, 0, i64);

  unsigned int Len = OldMMO->getSizeInBits().getValue();
  if (OrigTy.getScalarSizeInBits() != Len) {
    Register Wide = MRI.createGenericVirtualRegister(i64);
    MIRBuilder.buildTrunc(Dest, Wide);
    Dest = Wide;
  }

  if (NeedSExt) {
    Register Temp = MRI.createGenericVirtualRegister(i64);
    MIRBuilder.buildLoad(Temp, Addr, *NewMMO);
    MIRBuilder.buildSExtInReg(Dest, Temp, OldMMO->getSizeInBits().getValue());
  } else if (NeedZExt) {
    Register Temp = MRI.createGenericVirtualRegister(i64);
    MIRBuilder.buildLoad(Temp, Addr, *NewMMO);
    Register Mask = MRI.createGenericVirtualRegister(i64);
    MIRBuilder.buildConstant(Mask, (1 << Len) - 1);
    MIRBuilder.buildAnd(Dest, Temp, Mask);
  } else {
    MIRBuilder.buildLoad(Dest, Addr, *NewMMO);
    assert(OrigTy.getScalarSizeInBits() < 64);
  }

  MI.eraseFromParent();
  return true;
}

bool legalizeStore(MachineInstr &MI, MachineRegisterInfo &MRI,
                   MachineIRBuilder &MIRBuilder,
                   GISelChangeObserver &Observer) {
  MachineFunction *MF = MI.getMF();
  assert(MI.getOpcode() == TargetOpcode::G_STORE);

  Register Src = MI.getOperand(0).getReg();
  Register Addr = MI.getOperand(1).getReg();

  LLT SrcTy = MRI.getType(Src);
  assert(SrcTy.isScalar() && SrcTy.getSizeInBits() < 64);
  assert(MI.memoperands().size() == 1);

  MachineMemOperand *OldMMO = *MI.memoperands_begin();
  MachineMemOperand *NewMMO =
      MF->getMachineMemOperand(OldMMO, 0, LLT::integer(64));

  Register Wide = MRI.createGenericVirtualRegister(LLT::integer(64));
  MIRBuilder.buildAnyExt(Wide, Src);
  MIRBuilder.buildStore(Wide, Addr, *NewMMO);

  MI.eraseFromParent();
  return true;
}

void buildNanTest(Register Result, Register Val, bool TrueOnNan,
                  MachineIRBuilder &MIRBuilder, MachineRegisterInfo &MRI) {
  Register Dummy = MRI.createGenericVirtualRegister(LLT::float64());
  MRI.setRegClass(Dummy, &IC10::FPRRegClass);
  MIRBuilder.buildFConstant(Dummy, 0);
  CmpInst::Predicate Pred = TrueOnNan ? CmpInst::FCMP_UNO : CmpInst::FCMP_ORD;
  MIRBuilder.buildFCmp(Pred, Result, Val, Dummy);
}

bool legalizeFCMP(MachineInstr &MI, MachineRegisterInfo &MRI,
                  MachineIRBuilder &MIRBuilder, GISelChangeObserver &Observer) {
  assert(MI.getOpcode() == TargetOpcode::G_FCMP);
  auto &CmpMI = cast<GFCmp>(MI);
  CmpInst::Predicate Pred = CmpMI.getCond();
  if (Pred == CmpInst::FCMP_ORD || Pred == CmpInst::FCMP_UNO)
    return true;

  Register Dest = CmpMI.getOperand(0).getReg();

  bool OrderedAndCMP = CmpInst::isOrdered(Pred);

  bool CanPull = false;
  bool PullLHSPos = false;

  unsigned int GCMPOpcode = 0;
  switch (Pred) {
  default:
    llvm_unreachable("Unsupported predicate");
  case CmpInst::FCMP_OEQ:
  case CmpInst::FCMP_UEQ:
    GCMPOpcode = IC10::G_FEQ;
    break;
  case CmpInst::FCMP_ONE:
  case CmpInst::FCMP_UNE:
    GCMPOpcode = IC10::G_FNE;
    break;
  case CmpInst::FCMP_OGT: // a > b
    CanPull = true;
    PullLHSPos = false;
    GCMPOpcode = IC10::G_FGT;
    break;
  case CmpInst::FCMP_UGT: // a > b
    GCMPOpcode = IC10::G_FGT;
    break;
  case CmpInst::FCMP_OGE: // a >= b
    GCMPOpcode = IC10::G_FGE;
    break;
  case CmpInst::FCMP_UGE: // a >= b
    CanPull = true;
    PullLHSPos = true;
    GCMPOpcode = IC10::G_FGE;
    break;
  case CmpInst::FCMP_OLT: // a < b
    CanPull = true;
    PullLHSPos = true;
    GCMPOpcode = IC10::G_FLT;
    break;
  case CmpInst::FCMP_ULT: // a < b
    GCMPOpcode = IC10::G_FLT;
    break;
  case CmpInst::FCMP_OLE: // a <= b
    GCMPOpcode = IC10::G_FLE;
    break;
  case CmpInst::FCMP_ULE: // a <= b
    CanPull = true;
    PullLHSPos = false;
    GCMPOpcode = IC10::G_FLE;
    break;
  }

  Register LHS = CmpMI.getLHSReg();
  Register RHS = CmpMI.getRHSReg();

  if (MI.getFlags() & MachineInstr::MIFlag::FmNoInfs) {
    MIRBuilder.buildInstr(GCMPOpcode)
        .addReg(Dest, RegState::Define)
        .addReg(LHS)
        .addReg(RHS);
    MI.removeFromParent();
    return true;
  }

  // use pull to infinity trick if it can
  if (CanPull) {
    // build nan checks
    Register LHSIsNAN = MRI.createGenericVirtualRegister(LLT::integer(64));
    Register RHSIsNAN = MRI.createGenericVirtualRegister(LLT::integer(64));
    buildNanTest(LHSIsNAN, LHS, true, MIRBuilder, MRI);
    buildNanTest(RHSIsNAN, RHS, true, MIRBuilder, MRI);
    // create pulling constants
    Register LHSPullConst = MRI.createGenericVirtualRegister(LLT::float64());
    Register RHSPullConst = MRI.createGenericVirtualRegister(LLT::float64());
    MIRBuilder.buildFConstant(LHSPullConst, PullLHSPos ? INFINITY : -INFINITY);
    MIRBuilder.buildFConstant(RHSPullConst, PullLHSPos ? -INFINITY : INFINITY);
    // create filtered values
    Register PulledLHS = MRI.createGenericVirtualRegister(LLT::float64());
    Register PulledRHS = MRI.createGenericVirtualRegister(LLT::float64());
    MIRBuilder.buildSelect(PulledLHS, LHSIsNAN, LHSPullConst, LHS);
    MIRBuilder.buildSelect(PulledRHS, RHSIsNAN, RHSPullConst, RHS);
    // create compare function
    MIRBuilder.buildInstr(GCMPOpcode)
        .addReg(Dest, RegState::Define)
        .addReg(PulledLHS)
        .addReg(PulledRHS);
    MI.eraseFromParent();
    return true;
  }

  // the only thing left should be
  // EQ, NE, UGT, OGE, ULT, OLE

  // build nan checks
  Register LHSNANTest = MRI.createGenericVirtualRegister(LLT::integer(64));
  Register RHSNANTest = MRI.createGenericVirtualRegister(LLT::integer(64));
  Register JoinedNANTest = MRI.createGenericVirtualRegister(LLT::integer(64));
  buildNanTest(LHSNANTest, LHS, !OrderedAndCMP, MIRBuilder, MRI);
  buildNanTest(RHSNANTest, RHS, !OrderedAndCMP, MIRBuilder, MRI);
  if (OrderedAndCMP) {
    MIRBuilder.buildAnd(JoinedNANTest, LHSNANTest, RHSNANTest);
  } else {
    MIRBuilder.buildOr(JoinedNANTest, LHSNANTest, RHSNANTest);
  }
  // create nulling constant
  Register Zero = MRI.createGenericVirtualRegister(LLT::float64());
  MIRBuilder.buildFConstant(Zero, 0);
  // create filtered values
  Register FilteredLHS = MRI.createGenericVirtualRegister(LLT::float64());
  Register FilteredRHS = MRI.createGenericVirtualRegister(LLT::float64());
  if (OrderedAndCMP) {
    MIRBuilder.buildSelect(FilteredLHS, LHSNANTest, LHS, Zero);
    MIRBuilder.buildSelect(FilteredRHS, RHSNANTest, RHS, Zero);
  } else {
    MIRBuilder.buildSelect(FilteredLHS, LHSNANTest, Zero, LHS);
    MIRBuilder.buildSelect(FilteredRHS, RHSNANTest, Zero, RHS);
  }
  Register TmpResult = MRI.createGenericVirtualRegister(LLT::integer(64));
  MIRBuilder.buildInstr(GCMPOpcode)
      .addReg(TmpResult, RegState::Define)
      .addReg(FilteredLHS)
      .addReg(FilteredRHS);
  if (OrderedAndCMP) {
    MIRBuilder.buildAnd(Dest, TmpResult, JoinedNANTest);
  } else {
    MIRBuilder.buildOr(Dest, TmpResult, JoinedNANTest);
  }
  MI.eraseFromParent();
  return true;
}

} // namespace

bool IC10LegalizerInfo::legalizeCustom(
    LegalizerHelper &Helper, MachineInstr &MI,
    LostDebugLocObserver &LocObserver) const {

  MachineIRBuilder &MIRBuilder = Helper.MIRBuilder;
  MachineRegisterInfo &MRI = *MIRBuilder.getMRI();
  GISelChangeObserver &Observer = Helper.Observer;

  using namespace TargetOpcode;
  switch (MI.getOpcode()) {
  default:
    return false;
  case G_LOAD:
  case G_ZEXTLOAD:
  case G_SEXTLOAD:
    return legalizeLoad(MI, MRI, MIRBuilder, Observer);
  case G_STORE:
    return legalizeStore(MI, MRI, MIRBuilder, Observer);
  case G_FCMP:
    return legalizeFCMP(MI, MRI, MIRBuilder, Observer);
  }
}
