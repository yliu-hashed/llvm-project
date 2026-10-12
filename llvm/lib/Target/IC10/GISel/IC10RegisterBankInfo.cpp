// IC10

#include "IC10RegisterBankInfo.h"
#include "IC10RegisterInfo.h"
#include "MCTargetDesc/IC10MCTargetDesc.h"
#include "llvm/CodeGen/GlobalISel/GenericMachineInstrs.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/RegisterBank.h"
#include "llvm/CodeGen/RegisterBankInfo.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/IR/IntrinsicsIC10.h"
#include "llvm/Support/Casting.h"
#include <_abort.h>
#include <cassert>

#define GET_TARGET_REGBANK_IMPL
#include "IC10GenRegisterBank.inc"

using namespace llvm;

namespace llvm {
namespace IC10 {

const RegisterBankInfo::PartialMapping PartMappings[]{
    // clang-format off
    // GPR Partial Mapping
    {0, 64, GPRRegBank},
    {0, 64, FPRRegBank},
    // clang-format on
};

enum PartialMappingIdx {
  PMI_GPR = 0,
  PMI_FPR = 1,
};

const RegisterBankInfo::ValueMapping ValueMappings[] = {
    // invalid
    {nullptr, 0},
    // 3 operands in GPRs
    {&PartMappings[PMI_FPR], 1}, // 1: FGG
    {&PartMappings[PMI_GPR], 1}, // 2: GGG
    {&PartMappings[PMI_GPR], 1}, // 3:
    {&PartMappings[PMI_GPR], 1}, // 4:
    {&PartMappings[PMI_GPR], 1}, // 5: GFF
    {&PartMappings[PMI_FPR], 1}, // 6: FFF
    {&PartMappings[PMI_FPR], 1}, // 7:
    {&PartMappings[PMI_FPR], 1}, // 8: FGFF
    {&PartMappings[PMI_GPR], 1},
    {&PartMappings[PMI_FPR], 1},
    {&PartMappings[PMI_FPR], 1},
};

enum ValueMappingIdx {
  InvalidIdx = 0,
  VMI_FGG = 1,
  VMI_GGG = 2,
  VMI_GGGG = 2,
  VMI_GFF = 5,
  VMI_FFF = 6,
  VMI_FGFF = 8,
};

} // end namespace IC10
} // end namespace llvm

namespace {

bool isFloat(Register Reg, const MachineRegisterInfo &MRI) {
  // first, check pointer's type
  LLT Ty = MRI.getType(Reg);
  if (Ty.isFloat())
    return true;
  if (Ty.isInteger() || Ty.isPointer())
    return false;
  // then check register class
  if (Reg.isPhysical())
    return false;
  auto *RegClass = MRI.getRegClass(Reg);
  if (RegClass == &IC10::FPRRegClass)
    return true;
  if (RegClass == &IC10::GPRRegClass)
    return false;
  // don't know what to do
  return false;
}

} // namespace

IC10RegisterBankInfo::IC10RegisterBankInfo(const TargetRegisterInfo &RTI)
    : IC10GenRegisterBankInfo() {}

const RegisterBankInfo::InstructionMapping &
IC10RegisterBankInfo::getInstrMapping(const MachineInstr &MI) const {
  auto Opc = MI.getOpcode();

  if ((Opc != TargetOpcode::COPY && !isPreISelGenericOpcode(Opc)) ||
      Opc == TargetOpcode::G_PHI) {
    const InstructionMapping &Mapping = getInstrMappingImpl(MI);
    if (Mapping.isValid())
      return Mapping;
  }

  const MachineFunction &MF = *MI.getParent()->getParent();
  const MachineRegisterInfo &MRI = MF.getRegInfo();

  using namespace TargetOpcode;

  unsigned NumOperands = MI.isCopyLike() ? 1 : MI.getNumOperands();
  const ValueMapping *OperandsMapping = nullptr;

  const ValueMapping *SingleF = &IC10::ValueMappings[IC10::VMI_FFF];
  const ValueMapping *SingleG = &IC10::ValueMappings[IC10::VMI_GGG];

  switch (Opc) {
  case COPY: {
    bool IsFloat1 = isFloat(MI.getOperand(0).getReg(), MRI);
    bool IsFloat2 = isFloat(MI.getOperand(1).getReg(), MRI);
    if (IsFloat1 || IsFloat2) {
      OperandsMapping = &IC10::ValueMappings[IC10::VMI_FFF];
    } else {
      OperandsMapping = &IC10::ValueMappings[IC10::VMI_GGG];
    }
    break;
  }
  case G_PTR_ADD:
  case G_ADD:
  case G_SUB:
  case G_MUL:
  case G_SDIV:
  case G_UDIV:
  case G_SHL:
  case G_ASHR:
  case G_LSHR:
  case G_AND:
  case G_OR:
  case G_XOR:
    OperandsMapping = &IC10::ValueMappings[IC10::VMI_GGG];
    break;
  case G_ANYEXT:
  case G_TRUNC: {
    OperandsMapping = &IC10::ValueMappings[IC10::VMI_GGG];
    break;
  }
  case G_LOAD:
  case G_STORE: {
    LLT Ty = MRI.getType(MI.getOperand(0).getReg());
    if (Ty.isFloat()) {
      OperandsMapping = &IC10::ValueMappings[IC10::VMI_FGG];
    } else if (Ty.isInteger() || Ty.isPointer()) {
      OperandsMapping = &IC10::ValueMappings[IC10::VMI_GGG];
    } else {
      return getInvalidInstructionMapping();
    }
    break;
  }
  case G_SELECT: {
    LLT Ty = MRI.getType(MI.getOperand(0).getReg());
    if (Ty.isFloat()) {
      OperandsMapping = &IC10::ValueMappings[IC10::VMI_FGFF];
    } else if (Ty.isInteger() || Ty.isPointer()) {
      OperandsMapping = &IC10::ValueMappings[IC10::VMI_GGGG];
    } else {
      return getInvalidInstructionMapping();
    }
    break;
  }
  case G_CONSTANT:
  case G_FRAME_INDEX:
    OperandsMapping = getOperandsMapping({SingleG, nullptr});
    break;
  case G_SITOFP:
  case G_UITOFP:
    OperandsMapping = &IC10::ValueMappings[IC10::VMI_FGG];
    break;
  case G_FPTOSI:
  case G_FPTOUI:
    OperandsMapping = &IC10::ValueMappings[IC10::VMI_GFF];
    break;
  case G_FADD:
  case G_FSUB:
  case G_FMUL:
  case G_FDIV:
  case G_FMAXIMUM:
  case G_FMINIMUM:
  case G_FATAN2:
  case G_FABS:
  case G_INTRINSIC_TRUNC:
  case G_INTRINSIC_ROUND:
  case G_FFLOOR:
  case G_FCEIL:
  case G_FSIN:
  case G_FCOS:
  case G_FTAN:
  case G_FASIN:
  case G_FACOS:
  case G_FATAN:
  case G_FSQRT:
  case G_FPOW:
  case G_FLOG:
    OperandsMapping = &IC10::ValueMappings[IC10::VMI_FFF];
    break;
  case G_FCONSTANT:
    OperandsMapping = getOperandsMapping({SingleF, nullptr});
    break;
  case G_INTRINSIC:
    OperandsMapping = getIntrinsicOperandMapping(MI);
    break;
  case G_INTRINSIC_W_SIDE_EFFECTS:
    OperandsMapping = getIntrinsicWSideEffectsOperandMapping(MI);
    break;
  case G_TRAP:
  case G_BR:
    OperandsMapping = getOperandsMapping({nullptr});
    break;
  case G_BRCOND:
    OperandsMapping = getOperandsMapping({SingleG, nullptr});
    break;
  case G_ICMP:
    OperandsMapping = getOperandsMapping({SingleG, nullptr, SingleG, SingleG});
    break;
  case G_FCMP:
    OperandsMapping = getOperandsMapping({SingleG, nullptr, SingleF, SingleF});
    break;
  case IC10::G_FEQ:
  case IC10::G_FGE:
  case IC10::G_FGT:
  case IC10::G_FLE:
  case IC10::G_FLT:
  case IC10::G_FNE:
    OperandsMapping = &IC10::ValueMappings[IC10::VMI_GFF];
    break;
  default:
    return getInvalidInstructionMapping();
  }

  return getInstructionMapping(DefaultMappingID, /*Cost=*/1, OperandsMapping,
                               NumOperands);
}

const RegisterBankInfo::ValueMapping *
IC10RegisterBankInfo::getIntrinsicOperandMapping(const MachineInstr &MI) const {
  assert(MI.getOpcode() == TargetOpcode::G_INTRINSIC);
  dbgs() << "Did not implement regular intrinsic mapping";
  abort();
}

const RegisterBankInfo::ValueMapping *
IC10RegisterBankInfo::getIntrinsicWSideEffectsOperandMapping(
    const MachineInstr &MI) const {
  assert(MI.getOpcode() == TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS);
  switch (cast<GIntrinsic>(MI).getIntrinsicID()) {
  default:
    return nullptr;
  case Intrinsic::ic10_yield:
    return getOperandsMapping({nullptr});
  case Intrinsic::ic10_sleep:
    return getOperandsMapping({nullptr, &IC10::ValueMappings[IC10::VMI_FFF]});
  }
}

const RegisterBank &
IC10RegisterBankInfo::getRegBankFromRegClass(const TargetRegisterClass &RC,
                                             LLT Ty) const {
  switch (RC.getID()) {
  case IC10::GPRRegClassID:
    return getRegBank(IC10::GPRRegBankID);
  case IC10::FPRRegClassID:
    return getRegBank(IC10::FPRRegBankID);
  default:
    return IC10RegisterBankInfo::getRegBankFromRegClass(RC, Ty);
  }
}
