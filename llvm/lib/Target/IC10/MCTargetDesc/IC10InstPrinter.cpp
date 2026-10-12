// IC10
#include "IC10InstPrinter.h"
#include "IC10.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/NativeFormatting.h"
#include "llvm/Support/raw_ostream.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

// Include the auto-generated portion of the assembly writer.
#define GET_INSTRUCTION_NAME
#define PRINT_ALIAS_INSTR
#include "IC10GenAsmWriter.inc"

void IC10InstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                StringRef Annot, const MCSubtargetInfo &STI,
                                raw_ostream &O) {
  printInstruction(MI, Address, STI, O);
  printAnnotation(O, Annot);
}

void IC10InstPrinter::printRegName(raw_ostream &O, MCRegister Reg) {
  O << StringRef(getRegisterName(Reg));
}

void IC10InstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                   const MCSubtargetInfo &STI, raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);

  if (Op.isReg()) {
    printRegName(O, Op.getReg());
    return;
  }

  if (Op.isImm()) {
    O << Op.getImm();
    return;
  }

  if (Op.isDFPImm()) {
    double Val = bit_cast<double>(Op.getDFPImm());
    printDouble(Val, O);
    return;
  }

  if (Op.isSFPImm()) {
    float Val = bit_cast<float>(Op.getSFPImm());
    printDouble(Val, O);
    return;
  }

  if (Op.isExpr()) {
    MAI.printExpr(O, *Op.getExpr());
    return;
  }

  llvm_unreachable("Unknown operand");
}

void IC10InstPrinter::printDouble(double Value, raw_ostream &O) const {
  if (Value == NAN) {
    O << "NAN";
    return;
  }
  if (std::isnan(Value))
    llvm_unreachable("Cannot print NAN to immediate yet.");
  if (std::isinf(Value)) {
    O << (Value > 0 ? "INF" : "-INF");
    return;
  }
  // print an integer if can be represented by an integer
  if ((std::abs(Value) < 0x1000000000) && (std::trunc(Value) == Value)) {
    printInt((int64_t)Value, O);
    return;
  }
  O << Value;
  return;
}

void IC10InstPrinter::printInt(int64_t Value, raw_ostream &O) const {
  O << Value;
}
