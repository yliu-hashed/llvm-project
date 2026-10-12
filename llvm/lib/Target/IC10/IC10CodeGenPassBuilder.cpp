// IC10

#include "IC10.h"
#include "IC10AsmPrinter.h"
#include "IC10TargetMachine.h"
#include "llvm/CodeGen/GlobalISel/IRTranslator.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelect.h"
#include "llvm/CodeGen/GlobalISel/Legalizer.h"
#include "llvm/CodeGen/GlobalISel/RegBankSelect.h"
#include "llvm/CodeGen/MachineCombiner.h"
#include "llvm/IR/PassInstrumentation.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Passes/CodeGenPassBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class IC10CodeGenPassBuilder : public CodeGenPassBuilder {
  using Base = CodeGenPassBuilder;

  IC10TargetMachine &getTM() const {
    return static_cast<IC10TargetMachine &>(TM);
  }

public:
  explicit IC10CodeGenPassBuilder(IC10TargetMachine &TM,
                                  const CGPassBuilderOption &Opts,
                                  PassInstrumentationCallbacks *PIC)
      : CodeGenPassBuilder(TM, Opts, PIC) {
  }

  Error addIRTranslator(PassManagerWrapper &PMW) override;
  void addPreLegalizeMachineIR(PassManagerWrapper &PMW) override;
  Error addLegalizeMachineIR(PassManagerWrapper &PMW) override;
  void addPreRegBankSelect(PassManagerWrapper &PMW) override;
  Error addRegBankSelect(PassManagerWrapper &PMW) override;
  Error addGlobalInstructionSelect(PassManagerWrapper &PMW) override;
  void addPreEmitPass(PassManagerWrapper &PMW) override;

  // void addMachineLateOptimization(PassManagerWrapper &PMW) override;

  void addAsmPrinterBegin(PassManagerWrapper &PMW) override;
  void addAsmPrinter(PassManagerWrapper &PMW) override;
  void addAsmPrinterEnd(PassManagerWrapper &PMW) override;
};

Error IC10CodeGenPassBuilder::addIRTranslator(PassManagerWrapper &PMW) {
  addMachineFunctionPass(IRTranslatorPass(getOptLevel()), PMW);
  return Error::success();
}

void IC10CodeGenPassBuilder::addPreLegalizeMachineIR(PassManagerWrapper &PMW) {
  addMachineFunctionPass(IC10PreLegalizerAddressabilityFixPass(), PMW);
  addMachineFunctionPass(IC10PreLegalizerCombinerPass(), PMW);
}

Error IC10CodeGenPassBuilder::addLegalizeMachineIR(PassManagerWrapper &PMW) {
  addMachineFunctionPass(LegalizerPass(), PMW);
  return Error::success();
}

void IC10CodeGenPassBuilder::addPreRegBankSelect(PassManagerWrapper &PMW) {
  if (getOptLevel() != CodeGenOptLevel::None)
      addMachineFunctionPass(IC10PostLegalizerCombinerPass(), PMW);
}

Error IC10CodeGenPassBuilder::addRegBankSelect(PassManagerWrapper &PMW) {
  addMachineFunctionPass(RegBankSelectPass(), PMW);
  return Error::success();
}

Error IC10CodeGenPassBuilder::addGlobalInstructionSelect(
    PassManagerWrapper &PMW) {
  addMachineFunctionPass(InstructionSelectPass(getOptLevel()), PMW);
  return Error::success();
}

void IC10CodeGenPassBuilder::addPreEmitPass(PassManagerWrapper &PMW) {
  addMachineFunctionPass(IC10ExpandPseudoInstsPass(), PMW);
  addMachineFunctionPass(IC10LateCombinePushPopPass(), PMW);
}

void IC10CodeGenPassBuilder::addAsmPrinterBegin(PassManagerWrapper &PMW) {
  addModulePass(IC10AsmPrinterBeginPass(), PMW, /*Force=*/ false);
}

void IC10CodeGenPassBuilder::addAsmPrinter(PassManagerWrapper &PMW) {
  addMachineFunctionPass(IC10AsmPrinterPass(), PMW);
}

void IC10CodeGenPassBuilder::addAsmPrinterEnd(PassManagerWrapper &PMW) {
  addModulePass(IC10AsmPrinterEndPass(), PMW);
}

} // end namespace

void IC10TargetMachine::registerPassBuilderCallbacks(PassBuilder &PB) {
#define GET_PASS_REGISTRY "IC10PassRegistry.def"
#include "llvm/Passes/TargetPassRegistry.inc"
}

Error IC10TargetMachine::buildCodeGenPipeline(
    ModulePassManager &MPM, ModuleAnalysisManager &MAM, raw_pwrite_stream &Out,
    raw_pwrite_stream *DwoOut, CodeGenFileType FileType,
    const CGPassBuilderOption &Opt, MCContext &Ctx,
    PassInstrumentationCallbacks *PIC) {
  auto CGPB = IC10CodeGenPassBuilder(*this, Opt, PIC);
  return CGPB.buildPipeline(MPM, MAM, Out, DwoOut, FileType, Ctx);
}
