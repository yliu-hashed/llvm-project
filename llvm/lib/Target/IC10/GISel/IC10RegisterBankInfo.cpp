// IC10

#include "IC10RegisterBankInfo.h"
#include "IC10RegisterInfo.h"
#include "MCTargetDesc/IC10MCTargetDesc.h"
#include "llvm/CodeGen/GlobalISel/GenericMachineInstrs.h"
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
    {0, 64, APRRegBank},
    // clang-format on
};

enum PartialMappingIdx {
  PMI_APR = 0,
};

const RegisterBankInfo::ValueMapping ValueMappings[] = {
    // invalid
    {nullptr, 0},
    // 3 operands in APR
    {&PartMappings[PMI_APR], 1}, // 5G
    {&PartMappings[PMI_APR], 1},
    {&PartMappings[PMI_APR], 1},
    {&PartMappings[PMI_APR], 1},
    {&PartMappings[PMI_APR], 1},
    {nullptr, 0},
    {&PartMappings[PMI_APR], 1},
    {&PartMappings[PMI_APR], 1},
    {&PartMappings[PMI_APR], 1},
};

enum ValueMappingIdx {
  InvalidIdx = 0,
  VMI_5G = 1,
};

} // end namespace IC10
} // end namespace llvm

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

  using namespace TargetOpcode;

  unsigned NumOperands = MI.isCopyLike() ? 1 : MI.getNumOperands();
  const ValueMapping *OperandsMapping = nullptr;

  const ValueMapping *Single = &IC10::ValueMappings[IC10::VMI_5G];

  switch (Opc) {
  case COPY:
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
  case G_ANYEXT:
  case G_TRUNC:
  case G_LOAD:
  case G_STORE:
  case G_SELECT:
  case G_SITOFP:
  case G_UITOFP:
  case G_FPTOSI:
  case G_FPTOUI:
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
  case IC10::G_FEQ:
  case IC10::G_FGE:
  case IC10::G_FGT:
  case IC10::G_FLE:
  case IC10::G_FLT:
  case IC10::G_FNE:
    OperandsMapping = &IC10::ValueMappings[IC10::VMI_5G];
    break;
  case G_CONSTANT:
  case G_FCONSTANT:
  case G_FRAME_INDEX:
    OperandsMapping = getOperandsMapping({Single, nullptr});
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
    OperandsMapping = getOperandsMapping({Single, nullptr});
    break;
  case G_ICMP:
  case G_FCMP:
    OperandsMapping = getOperandsMapping({Single, nullptr, Single, Single});
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
    return getOperandsMapping({nullptr, &IC10::ValueMappings[IC10::VMI_5G]});
  }
}

const RegisterBank &
IC10RegisterBankInfo::getRegBankFromRegClass(const TargetRegisterClass &RC,
                                             LLT Ty) const {
  switch (RC.getID()) {
  case IC10::GPRRegClassID:
  case IC10::FPRRegClassID:
    return getRegBank(IC10::APRRegBankID);
  default:
    return IC10RegisterBankInfo::getRegBankFromRegClass(RC, Ty);
  }
}
