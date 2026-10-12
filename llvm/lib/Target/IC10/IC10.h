// IC10

#ifndef LLVM_LIB_TARGET_IC10_IC10_H
#define LLVM_LIB_TARGET_IC10_IC10_H

#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {

class InstructionSelector;
class IC10RegisterBankInfo;
class IC10Subtarget;
class IC10TargetMachine;
class PassRegistry;

InstructionSelector *
createIC10InstructionSelector(const IC10TargetMachine &, const IC10Subtarget &,
                              const IC10RegisterBankInfo &);

void initializeIC10AsmPrinterPass(PassRegistry &);

class IC10PreLegalizerAddressabilityFixPass
    : public RequiredPassInfoMixin<IC10PreLegalizerAddressabilityFixPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

class IC10PreLegalizerCombinerPass
    : public RequiredPassInfoMixin<IC10PreLegalizerCombinerPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

class IC10PostLegalizerCombinerPass
    : public RequiredPassInfoMixin<IC10PostLegalizerCombinerPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

class IC10ExpandPseudoInstsPass
    : public RequiredPassInfoMixin<IC10ExpandPseudoInstsPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

class IC10LateCombinePushPopPass
    : public RequiredPassInfoMixin<IC10LateCombinePushPopPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_IC10_IC10_H
