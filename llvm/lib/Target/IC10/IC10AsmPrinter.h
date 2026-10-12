// IC10

#ifndef LLVM_LIB_TARGET_IC10_IC10ASMPRINTER_H
#define LLVM_LIB_TARGET_IC10_IC10ASMPRINTER_H

#include "IC10MachineFunctionInfo.h"
#include "IC10Subtarget.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunctionAnalysisManager.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/PassManager.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Target/TargetMachine.h"
#include <cstddef>


namespace llvm {

class LLVM_LIBRARY_VISIBILITY IC10AsmPrinter : public AsmPrinter {
public:
  static char ID;

private:
  const IC10Subtarget *Subtarget;
  const MachineRegisterInfo *MRI;
  IC10MachineFunctionInfo *MFI;

public:
  explicit IC10AsmPrinter(TargetMachine &TM,
                          std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {
    Subtarget = nullptr;
    MRI = nullptr;
    MFI = nullptr;
  }

  StringRef getPassName() const override {
    return "IC10 Assembly Printer";
  }

  const IC10Subtarget &getSubtarget() const { return *Subtarget; }

  bool runOnMachineFunction(MachineFunction &MF) override {
    Subtarget = &MF.getSubtarget<IC10Subtarget>();
    MRI = &MF.getRegInfo();
    MFI = MF.getInfo<IC10MachineFunctionInfo>();
    return AsmPrinter::runOnMachineFunction(MF);
  }

  void emitFunctionEntryLabel() override;
  void emitInstruction(const MachineInstr *MI) override;
  void emitFunctionBodyStart() override;

private:
  void expandCallInst(const MachineInstr *MI);
  void expandRetInst();
};


class IC10AsmPrinterBeginPass
    : public RequiredPassInfoMixin<IC10AsmPrinterBeginPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

class IC10AsmPrinterPass
    : public RequiredPassInfoMixin<IC10AsmPrinterPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

class IC10AsmPrinterEndPass
    : public RequiredPassInfoMixin<IC10AsmPrinterEndPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

} // end namespace llvm






#endif // LLVM_LIB_TARGET_IC10_IC10ASMPRINTER_H
