// IC10

#include "IC10AsmPrinter.h"
#include "IC10MCInstLower.h"
#include "TargetInfo/IC10TargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/AsmPrinterAnalysis.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/IR/Analysis.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

void IC10AsmPrinter::emitFunctionBodyStart() {}

void IC10AsmPrinter::emitFunctionEntryLabel() {
  OutStreamer->emitLabel(CurrentFnSym);
}

void IC10AsmPrinter::emitInstruction(const MachineInstr *MI) {
  MachineBasicBlock::const_instr_iterator I = MI->getIterator();
  MachineBasicBlock::const_instr_iterator E = MI->getParent()->instr_end();

  do {
    // switch (I->getOpcode()) {
    //   case IC10::CALL_PSEUDO:
    //     expandCallInst(&*I);
    //     return;
    //   case IC10::RET_PSEUDO:
    //     expandRetInst();
    //     return;
    // }

    MCInst TmpInst;
    IC10MCInstLower MCInstLowering(OutContext, *this);
    MCInstLowering.Lower(&*I, TmpInst);
    EmitToStreamer(*OutStreamer, TmpInst);
  } while ((++I != E) && I->isInsideBundle());
}

void IC10AsmPrinter::expandCallInst(const MachineInstr *MI) {
  // MCInst TmpInst;
  // MCInstLowering.Lower(MI, TmpInst);

  // Register CallAddrReg = TmpInst.getOperand(0).getReg();

  // Register TmpReg = IC10::R4;

  // // Address that SP points to will contain the return address.
  // // put return address into temporary

  // // get return addr
  // EmitToStreamer(*OutStreamer,
  // MCInstBuilder(IC10::LI).addReg(TmpReg).addImm(3));
  // EmitToStreamer(*OutStreamer,
  // MCInstBuilder(IC10::ADD_R).addReg(TmpReg).addReg(TmpReg).addReg(IC10::PC));
  // // store return addr
  // EmitToStreamer(*OutStreamer,
  // MCInstBuilder(IC10::SQ).addReg(TmpReg).addReg(IC10::SP).addImm(0));
  // // jump
  // EmitToStreamer(*OutStreamer,
  // MCInstBuilder(IC10::OR_J).addReg(CallAddrReg).addReg(CallAddrReg));
}

void IC10AsmPrinter::expandRetInst() {
  // EmitToStreamer(*OutStreamer,
  // MCInstBuilder(IC10::LQ).addReg(IC10::PC).addReg(IC10::SP).addImm(0));
}

char IC10AsmPrinter::ID = 0;
INITIALIZE_PASS(IC10AsmPrinter, "ic10-asm-printer", "IC10 Assmebly Printer",
                false, false)

// Force static initialization.
extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeIC10AsmPrinter() {
  RegisterAsmPrinter<IC10AsmPrinter> X(getTheIC10Target());
}

PreservedAnalyses IC10AsmPrinterBeginPass::run(Module &M,
                                               ModuleAnalysisManager &MAM) {
  IC10AsmPrinter &AsmPrinter = static_cast<IC10AsmPrinter &>(
      MAM.getResult<AsmPrinterAnalysis>(M).getPrinter());
  setupModuleAsmPrinter(M, MAM, AsmPrinter);
  AsmPrinter.doInitialization(M);
  return PreservedAnalyses::all();
}

PreservedAnalyses
IC10AsmPrinterPass::run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM) {
  IC10AsmPrinter &AsmPrinter = static_cast<IC10AsmPrinter &>(
      MFAM.getResult<ModuleAnalysisManagerMachineFunctionProxy>(MF)
          .getCachedResult<AsmPrinterAnalysis>(*MF.getFunction().getParent())
          ->getPrinter());
  setupMachineFunctionAsmPrinter(MFAM, MF, AsmPrinter);
  AsmPrinter.runOnMachineFunction(MF);
  return PreservedAnalyses::all();
}

PreservedAnalyses IC10AsmPrinterEndPass::run(Module &M,
                                             ModuleAnalysisManager &MAM) {
  IC10AsmPrinter &AsmPrinter = static_cast<IC10AsmPrinter &>(
      MAM.getResult<AsmPrinterAnalysis>(M).getPrinter());
  setupModuleAsmPrinter(M, MAM, AsmPrinter);
  AsmPrinter.doFinalization(M);
  return PreservedAnalyses::all();
}
