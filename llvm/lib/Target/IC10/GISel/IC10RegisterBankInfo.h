// IC10

#ifndef LLVM_LIB_TARGET_IC10_GLSEL_IC10REGISTERBANKINFO_H
#define LLVM_LIB_TARGET_IC10_GLSEL_IC10REGISTERBANKINFO_H

#include "llvm/CodeGen/RegisterBankInfo.h"

#define GET_REGBANK_DECLARATIONS
#include "IC10GenRegisterBank.inc"
#undef GET_REGBANK_DECLARATIONS

namespace llvm {

class TargetRegisterInfo;

class IC10GenRegisterBankInfo : public RegisterBankInfo {
protected:
#define GET_TARGET_REGBANK_CLASS
#include "IC10GenRegisterBank.inc"
#undef GET_TARGET_REGBANK_CLASS
};

/// This class provides the information for the target register banks.
class IC10RegisterBankInfo final : public IC10GenRegisterBankInfo {
public:
  IC10RegisterBankInfo(const TargetRegisterInfo &TRI);

  const InstructionMapping &
  getInstrMapping(const MachineInstr &MI) const override;

  const RegisterBank &getRegBankFromRegClass(const TargetRegisterClass &RC,
                                             LLT Ty) const override;

private:
  const ValueMapping *getIntrinsicOperandMapping(const MachineInstr &MI) const;
  const ValueMapping *
  getIntrinsicWSideEffectsOperandMapping(const MachineInstr &MI) const;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_IC10_GLSEL_IC10REGISTERBANKINFO_H
