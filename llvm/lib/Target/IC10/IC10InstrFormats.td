// IC10

//===----------------------------------------------------------------------===//
// Operand Definitions
//===----------------------------------------------------------------------===//

def uimm32 : Operand<i64>, ImmLeaf<i64, [{
  return Imm > -4294967296 && Imm < 4294967296;
}]>;

def fimm64 : Operand<f64>, FPImmLeaf<f64, [{
  return true;
}]>;

class IC10Inst<dag outs, dag ins, string asmstr> : Instruction {
  let Namespace = "IC10";
  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let Size = 8;
}

class IC10PseudoInst<dag outs, dag ins, string asmstr> :
    IC10Inst<outs, ins, asmstr> {
  let isPseudo = 1;
  let isCodeGenOnly = 1;
}
