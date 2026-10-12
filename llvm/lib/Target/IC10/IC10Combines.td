// IC10

include "llvm/Target/GlobalISel/Combine.td"

def IC10PreLegalizerCombiner : GICombiner<
  "IC10PreLegalizerCombinerImpl", [all_combines]> {
}

def IC10O0PreLegalizerCombiner : GICombiner<
  "IC10O0PreLegalizerCombinerImpl", [optnone_combines]> {
}

class not_asserted_fcmp<Instruction Op1, Instruction Op2> : GICombineRule<
  (defs root:$dst),
  (match (Op1 $neg, $x, $y),
         (G_XOR $dst, $neg, 1)),
  (apply (Op2 $dst, $x, $y))
>;

def not_asserted_fne : not_asserted_fcmp<G_FNE, G_FEQ>;
def not_asserted_feq : not_asserted_fcmp<G_FEQ, G_FNE>;
def not_asserted_fge : not_asserted_fcmp<G_FGE, G_FLT>;
def not_asserted_fgt : not_asserted_fcmp<G_FGT, G_FLE>;
def not_asserted_fle : not_asserted_fcmp<G_FLE, G_FLT>;
def not_asserted_flt : not_asserted_fcmp<G_FLT, G_FLE>;

def not_asserted_fcmp_combines : GICombineGroup<[
  not_asserted_fne,
  not_asserted_feq,
  not_asserted_fge,
  not_asserted_fgt,
  not_asserted_fle,
  not_asserted_flt
]>;

// Post-legalization combines which are primarily optimizations.
def IC10PostLegalizerCombiner : GICombiner<
  "IC10PostLegalizerCombinerImpl",
      [copy_prop, constant_fold_binops, constant_fold_fp_ops,
       hoist_logic_op_with_same_opcode_hands,
       redundant_and, redundant_or, select_combines, select_zero_true,
       select_zero_false, select_not, cast_combines, canonicalize_icmp,
       canonicalize_fcmp, not_asserted_fcmp_combines]> {
}
