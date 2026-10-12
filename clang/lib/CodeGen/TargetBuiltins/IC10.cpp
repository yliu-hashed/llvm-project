// IC10

#include "CGBuiltin.h"
#include "clang/AST/TypeBase.h"
#include "clang/Basic/TargetBuiltins.h"
#include "llvm/IR/IntrinsicsIC10.h"
#include <cassert>

using namespace clang;
using namespace CodeGen;
using namespace llvm;

Value *CodeGenFunction::EmitIC10BuiltinExpr(unsigned BuiltinID,
                                            const CallExpr *E) {
  switch (BuiltinID) {
  default:
    return nullptr;
  case IC10::BI__builtin_ic10_yield:
    return Builder.CreateCall(CGM.getIntrinsic(Intrinsic::ic10_yield));
  case IC10::BI__builtin_ic10_sleep: {
    Value *Arg0 = EmitScalarExpr(E->getArg(0));
    return Builder.CreateCall(CGM.getIntrinsic(Intrinsic::ic10_sleep), Arg0);
  }
  }
}
