// IC10
#ifndef LLVM_LIB_TARGET_IC10_MCTARGETDESC_IC10MCTARGETDESC_H
#define LLVM_LIB_TARGET_IC10_MCTARGETDESC_IC10MCTARGETDESC_H

#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCTargetOptions.h"

#include <memory>

namespace llvm {
class Target;
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectWriter;
class MCRegisterInfo;
class MCSubtargetInfo;

class StringRef;
class raw_ostream;
class raw_pwrite_stream;
class Triple;

Target &getTheIC10Target();

// std::unique_ptr<MCObjectTargetWriter> createIC10ELFObjectWriter();

} // end namespace llvm

// Defines symbolic names for IC10 registers.  This defines a mapping from
// register name to register number.
//
#define GET_REGINFO_ENUM
#include "IC10GenRegisterInfo.inc"

// Defines symbolic names for the IC10 instructions.
//
#define GET_INSTRINFO_ENUM
#include "IC10GenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "IC10GenSubtargetInfo.inc"

#endif
