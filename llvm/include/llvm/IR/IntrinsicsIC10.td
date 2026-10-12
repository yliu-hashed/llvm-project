// IC10

let TargetPrefix = "ic10" in {  // All intrinsics start with "llvm.ic10.".

// llvm_i64_ty, llvm_double_ty, llvm_void_ty

def int_ic10_yield : ClangBuiltin<"__builtin_ic10_yield">,
  Intrinsic<[], [], [IntrHasSideEffects]>;

def int_ic10_sleep : ClangBuiltin<"__builtin_ic10_sleep">,
  Intrinsic<[], [llvm_double_ty], [IntrHasSideEffects]>;

def int_ic10_halt_and_catch_fire :
  ClangBuiltin<"__builtin_ic10_halt_and_catch_fire">,
  Intrinsic<[], [], [IntrNoMem, IntrNoReturn, IntrHasSideEffects]>;

} // let TargetPrefix = "ic10"
