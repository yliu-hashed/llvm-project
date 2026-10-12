// IC10
#ifndef LLVM_LIB_TARGET_IC10_FIXUPKINDS_H
#define LLVM_LIB_TARGET_IC10_FIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace IC10 {

// This table *must* be in the same order of
// MCFixupKindInfo Infos[IC10::NumTargetFixupKinds]
// in IC10AsmBackend.cpp.
enum Fixups {
  IC10FixupNone = FirstTargetFixupKind,
  IC10OrdinaryFixup,
  // Marker
  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};
} // namespace IC10
} // namespace llvm

#endif
