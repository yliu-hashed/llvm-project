// IC10
#ifndef LLVM_LIB_TARGET_IC10_IC10MCINSTLOWER_H
#define LLVM_LIB_TARGET_IC10_IC10MCINSTLOWER_H

#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/Support/Compiler.h"

namespace llvm {
class MCContext;
class MCInst;
class MCOperand;
class MachineInstr;
class MachineFunction;
class AsmPrinter;

class LLVM_LIBRARY_VISIBILITY IC10MCInstLower {
  MCContext &Ctx;
  AsmPrinter &Printer;

public:
  IC10MCInstLower(MCContext &Ctx, AsmPrinter &Printer)
      : Ctx(Ctx), Printer(Printer) {}
  void Lower(const MachineInstr *MI, MCInst &OutMI) const;

  MCOperand LowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym) const;

  MCSymbol *GetGlobalAddressSymbol(const MachineOperand &MO) const;
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_IC10_IC10MCINSTLOWER_H
