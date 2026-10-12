// IC10

#include "IC10.h"
#include "clang/Basic/MacroBuilder.h"
#include "clang/Basic/TargetBuiltins.h"

using namespace clang;
using namespace clang::targets;

IC10TargetInfo::IC10TargetInfo(const llvm::Triple &Triple,
                               const TargetOptions &) : TargetInfo(Triple) {
  resetDataLayout();

  NoAsmVariants = true;
  BoolWidth = 32;
  ShortWidth = 16;
  IntWidth = LongWidth = LongLongWidth = 32;
  BoolAlign = ShortAlign = IntAlign = LongAlign = LongLongAlign = 64;
  FloatWidth = 32;
  DoubleWidth = LongDoubleWidth = 64;
  FloatAlign = DoubleAlign = LongDoubleAlign = 64;
  PointerWidth = 64;
  PointerAlign = 64;
  SuitableAlign = 64;
  Int128Align = 64;

  SizeType = UnsignedInt;
  PtrDiffType = SignedInt;
  IntPtrType = SignedInt;

  HasStrictFP = false;
}

ArrayRef<const char *> IC10TargetInfo::getGCCRegNames() const {
  // clang-format off
  static const char *const GCCRegNames[] = {
      "r0",  "r1",  "r2",  "r3",
      "r4",  "r5",  "r6",  "r7",
      "r8",  "r9",  "r10", "r11",
      "r12", "r13", "r14", "r15",
      "sp",  "ra"
    };
  // clang-format on
  return llvm::ArrayRef(GCCRegNames);
}

void IC10TargetInfo::getTargetDefines(const LangOptions &Opts,
                                      MacroBuilder &Builder) const {
  Builder.defineMacro("__IC10__");
  Builder.defineMacro("__ic10__");
}

static constexpr int NumBuiltins =
    clang::IC10::LastTSBuiltin - Builtin::FirstTSBuiltin;

#define GET_BUILTIN_STR_TABLE
#include "clang/Basic/BuiltinsIC10.inc"
#undef GET_BUILTIN_STR_TABLE

// static constexpr Builtin::Info BuiltinInfos[] = {
// #define GET_BUILTIN_INFOS
// #include "clang/Basic/BuiltinsIC10.inc"
// #undef GET_BUILTIN_INFOS
// };

static constexpr Builtin::Info PrefixedBuiltinInfos[] = {
#define GET_BUILTIN_PREFIXED_INFOS
#include "clang/Basic/BuiltinsIC10.inc"
#undef GET_BUILTIN_PREFIXED_INFOS
};

// if this fails, then perhaps we have some regular "non prefixed" builtins
static_assert(std::size(PrefixedBuiltinInfos) == NumBuiltins);

llvm::SmallVector<Builtin::InfosShard>
IC10TargetInfo::getTargetBuiltins() const {
  return {{&BuiltinStrings, PrefixedBuiltinInfos, "__builtin_ic10_"}};
}
