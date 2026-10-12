// IC10
#ifndef LLVM_LIB_TARGET_IC10_IC10TARGETMACHINE_H
#define LLVM_LIB_TARGET_IC10_IC10TARGETMACHINE_H

#include "IC10InstrInfo.h"
#include "IC10Subtarget.h"
#include "MCTargetDesc/IC10MCTargetDesc.h"
#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class IC10TargetMachine : public CodeGenTargetMachineImpl {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  IC10Subtarget Subtarget;

public:
  IC10TargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                    StringRef FS, const TargetOptions &Options,
                    std::optional<Reloc::Model> RM,
                    std::optional<CodeModel::Model> CM, CodeGenOptLevel OL,
                    bool JIT);

  ~IC10TargetMachine() override;

  const IC10Subtarget *getSubtargetImpl(const Function &) const override;

  void registerPassBuilderCallbacks(PassBuilder &PB) override;
  Error buildCodeGenPipeline(ModulePassManager &MPM, ModuleAnalysisManager &MAM,
                             raw_pwrite_stream &Out, raw_pwrite_stream *DwoOut,
                             CodeGenFileType FileType,
                             const CGPassBuilderOption &Opt, MCContext &Ctx,
                             PassInstrumentationCallbacks *PIC) override;

  bool shouldDefaultToNewPM() const override { return true; }

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_IC10_IC10TARGETMACHINE_H
