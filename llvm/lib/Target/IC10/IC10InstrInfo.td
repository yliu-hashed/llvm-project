// IC10
include "IC10InstrFormats.td"

class IC10GenericInstruction : GenericInstruction {
  let Namespace = "IC10";
}

class G_ASSERT_ORDERED_FCMP : IC10GenericInstruction {
  let OutOperandList = (outs type0:$dst);
  let InOperandList = (ins type1:$src1, type1:$src2);
  let hasSideEffects = 0;
  let GISelMatchGenericTypes = 1;
}

def G_FEQ : G_ASSERT_ORDERED_FCMP;
def G_FNE : G_ASSERT_ORDERED_FCMP;
def G_FGE : G_ASSERT_ORDERED_FCMP;
def G_FGT : G_ASSERT_ORDERED_FCMP;
def G_FLE : G_ASSERT_ORDERED_FCMP;
def G_FLT : G_ASSERT_ORDERED_FCMP;

def G_PUSH : IC10GenericInstruction {
  let OutOperandList = (outs ptype0:$inc);
  let InOperandList = (ins type1:$src, ptype0:$ptr);
  let hasSideEffects = 1;
  let mayStore = 1;
  let GISelMatchGenericTypes = 1;
}

def G_POP : IC10GenericInstruction {
  let OutOperandList = (outs type0:$dst, ptype0:$inc);
  let InOperandList = (ins ptype0:$ptr);
  let mayLoad = 1;
  let GISelMatchGenericTypes = 1;
}

def G_PEAK : IC10GenericInstruction {
  let OutOperandList = (outs type0:$dst);
  let InOperandList = (ins ptype0:$ptr);
  let mayLoad = 1;
  let GISelMatchGenericTypes = 1;
}

//===----------------------------------------------------------------------===//
// Type Profiles
//===----------------------------------------------------------------------===//

def BranchTarget : Operand<OtherVT>;

def AddrRegImm : ComplexPattern<iPTR, 2, "SelectAddrRegImm">;

def GIAddrRegImm :
  GIComplexOperandMatcher<s64, "selectAddrRegImm">,
  GIComplexPatternEquiv<AddrRegImm>;

// account for flog missing
def flog : SDNode<"ISD::FLOG", SDTFPUnaryOp>;
def : GINodeEquiv<G_FLOG, flog>;

//===----------------------------------------------------------------------===//
// Data Loading & Movement
//===----------------------------------------------------------------------===//

def MOVEi : IC10Inst<(outs FPR:$dst), (ins fimm64:$imm),
                     "move $dst $imm"> {
  let Pattern = [(set FPR:$dst, fimm64:$imm)];
}

def MOVEr : IC10Inst<(outs FPR:$dst), (ins FPR:$src),
                     "move $dst $src"> {
  let Pattern = [(set FPR:$dst, FPR:$src)];
}

def IMOVEi : IC10PseudoInst<(outs GPR:$dst), (ins uimm32:$imm),
                            "# int move $dst $imm"> {
  let Pattern = [(set GPR:$dst, uimm32:$imm)];
}

def IMOVEr : IC10PseudoInst<(outs GPR:$dst), (ins GPR:$src),
                            "# int move $dst $src"> {
  let Pattern = [(set GPR:$dst, GPR:$src)];
}

//===----------------------------------------------------------------------===//
// Int Arithmatics
//===----------------------------------------------------------------------===//

// Int Binary Arith

multiclass IC10Inst_IntBinArithClass<string opstr, bit rev> {
  def rr : IC10PseudoInst<(outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
                          "# int " # opstr # " $dst $src1 $src2">;
  def ri : IC10PseudoInst<(outs GPR:$dst), (ins GPR:$src1, uimm32:$imm2),
                          "# int " # opstr # " $dst $src1 $imm2">;
  if rev then {
    def ir : IC10PseudoInst<(outs GPR:$dst), (ins uimm32:$imm1, GPR:$src2),
                            "# int " # opstr # " $dst $imm1 $src2">;
  }
}

defm IADD : IC10Inst_IntBinArithClass<"add", 0>;
defm ISUB : IC10Inst_IntBinArithClass<"sub", 1>;
defm IMUL : IC10Inst_IntBinArithClass<"mul", 0>;
defm IAND : IC10Inst_IntBinArithClass<"and", 0>;
defm IOR  : IC10Inst_IntBinArithClass<"or",  0>;
defm INOR : IC10Inst_IntBinArithClass<"nor", 0>;
defm IXOR : IC10Inst_IntBinArithClass<"xor", 0>;
defm ISLL : IC10Inst_IntBinArithClass<"sll", 1>;
defm ISRA : IC10Inst_IntBinArithClass<"sra", 1>;
defm ISRL : IC10Inst_IntBinArithClass<"srl", 1>;

multiclass IC10Inst_IntBinArithPats<SDNode op, string inst, bit reverseImm> {
  def : Pat<(op GPR:$src1, GPR:$src2),
            (!cast<Instruction>(inst # "rr") GPR:$src1, GPR:$src2)>;
  def : Pat<(op GPR:$src1, uimm32:$imm2),
            (!cast<Instruction>(inst # "ri") GPR:$src1, uimm32:$imm2)>;
  if reverseImm then {
    def : Pat<(op uimm32:$imm1, GPR:$src2),
              (!cast<Instruction>(inst # "ir") uimm32:$imm1, GPR:$src2)>;
  }
}

defm : IC10Inst_IntBinArithPats<add, "IADD", 0>;
defm : IC10Inst_IntBinArithPats<sub, "ISUB", 1>;
defm : IC10Inst_IntBinArithPats<and, "IAND", 0>;
defm : IC10Inst_IntBinArithPats<or,  "IOR",  0>;
defm : IC10Inst_IntBinArithPats<xor, "IXOR", 0>;
defm : IC10Inst_IntBinArithPats<shl, "ISLL", 1>;
defm : IC10Inst_IntBinArithPats<sra, "ISRA", 1>;
defm : IC10Inst_IntBinArithPats<srl, "ISRL", 1>;

// Int Unary Arith

multiclass IC10Inst_IntUnaArithClass<string opstr> {
  def r: IC10PseudoInst<(outs GPR:$dst), (ins GPR:$src),
                         "# int " # opstr # " $dst $src">;
}

defm INOT : IC10Inst_IntUnaArithClass<"not">;
defm IABS : IC10Inst_IntUnaArithClass<"abs">;

multiclass IC10Inst_IntUnaArithPats<SDNode op, string inst> {
  def : Pat<(op GPR:$src),
            (!cast<Instruction>(inst # "r") GPR:$src)>;
}

defm : IC10Inst_IntUnaArithPats<abs, "IABS">;

// select instructions

def ISELECTrr : IC10PseudoInst<(outs GPR:$dst),
                               (ins GPR:$sel, GPR:$src1, GPR:$src2),
                               "select $dst $sel $src1 $src2">;
def ISELECTri : IC10PseudoInst<(outs GPR:$dst),
                               (ins GPR:$sel, GPR:$src1, uimm32:$src2),
                               "select $dst $sel $src1 $src2">;
def ISELECTir : IC10PseudoInst<(outs GPR:$dst),
                               (ins GPR:$sel, uimm32:$src1, GPR:$src2),
                               "select $dst $sel $src1 $src2">;
def ISELECTii : IC10PseudoInst<(outs GPR:$dst),
                               (ins GPR:$sel, uimm32:$src1, uimm32:$src2),
                               "select $dst $sel $src1 $src2">;
def : Pat<(select GPR:$sel, GPR:$src1, GPR:$src2),
          (ISELECTrr GPR:$sel, GPR:$src1, GPR:$src2)>;
def : Pat<(select GPR:$sel, GPR:$src1, uimm32:$src2),
          (ISELECTri GPR:$sel, GPR:$src1, uimm32:$src2)>;
def : Pat<(select GPR:$sel, uimm32:$src1, GPR:$src2),
          (ISELECTir GPR:$sel, uimm32:$src1, GPR:$src2)>;
def : Pat<(select GPR:$sel, uimm32:$src1, uimm32:$src2),
          (ISELECTii GPR:$sel, uimm32:$src1, uimm32:$src2)>;

// special patterns

def : Pat<(ptradd GPR:$src1, GPR:$src2), (IADDrr GPR:$src1, GPR:$src2)>;
def : Pat<(ptradd GPR:$src1, uimm32:$imm2), (IADDri GPR:$src1, uimm32:$imm2)>;
def : Pat<(frameindex:$fi), (IADDri (to_tframeindex $fi), 0)>;

//===----------------------------------------------------------------------===//
// Float Arithmatics
//===----------------------------------------------------------------------===//

// Float Binary Arith

multiclass IC10Inst_FloatBinArithClass<string opstr, bit revImm> {
  def rr: IC10Inst<(outs FPR:$dst), (ins FPR:$src1, FPR:$src2),
                    opstr # " $dst $src1 $src2">;
  def ri: IC10Inst<(outs FPR:$dst), (ins FPR:$src1, fimm64:$imm2),
                   opstr # " $dst $src1 $imm2">;
  if revImm then {
    def ir: IC10Inst<(outs FPR:$dst), (ins fimm64:$imm1, FPR:$src2),
                     opstr # " $dst $imm1 $src2">;
  }
}

defm ADD : IC10Inst_FloatBinArithClass<"add", 0>;
defm SUB : IC10Inst_FloatBinArithClass<"sub", 1>;
defm MUL : IC10Inst_FloatBinArithClass<"mul", 0>;
defm DIV : IC10Inst_FloatBinArithClass<"div", 1>;
defm POW : IC10Inst_FloatBinArithClass<"pow", 1>;
defm MAX : IC10Inst_FloatBinArithClass<"max", 0>;
defm MIN : IC10Inst_FloatBinArithClass<"min", 0>;

defm ATAN2 : IC10Inst_FloatBinArithClass<"atan2", 1>;

// actual integers instructions on the fp hardware (psudo expands to these)
defm AND : IC10Inst_FloatBinArithClass<"and", 0>;
defm OR  : IC10Inst_FloatBinArithClass<"or",  0>;
defm NOR : IC10Inst_FloatBinArithClass<"nor", 0>;
defm XOR : IC10Inst_FloatBinArithClass<"xor", 0>;
defm SLL : IC10Inst_FloatBinArithClass<"sll", 1>;
defm SRA : IC10Inst_FloatBinArithClass<"sra", 1>;
defm SRL : IC10Inst_FloatBinArithClass<"srl", 1>;

multiclass IC10Inst_FloatBinArithPats<SDNode op, string inst, bit revImm> {
  def : Pat<(op FPR:$src1, FPR:$src2),
            (!cast<Instruction>(inst # "rr") FPR:$src1, FPR:$src2)>;
  def : Pat<(op FPR:$src1, fimm64:$imm2),
            (!cast<Instruction>(inst # "ri") FPR:$src1, fimm64:$imm2)>;
  if revImm then {
    def : Pat<(op fimm64:$imm1, FPR:$src2),
              (!cast<Instruction>(inst # "ir") fimm64:$imm1, FPR:$src2)>;
  }
}

defm : IC10Inst_FloatBinArithPats<fadd, "ADD", 0>;
defm : IC10Inst_FloatBinArithPats<fsub, "SUB", 1>;
defm : IC10Inst_FloatBinArithPats<fmul, "MUL", 0>;
defm : IC10Inst_FloatBinArithPats<fdiv, "DIV", 1>;
defm : IC10Inst_FloatBinArithPats<fpow, "POW", 1>;
defm : IC10Inst_FloatBinArithPats<fmaximum, "MAX", 0>;
defm : IC10Inst_FloatBinArithPats<fminimum, "MIN", 0>;
defm : IC10Inst_FloatBinArithPats<fatan2, "ATAN2", 1>;

// Float Unary Arith

multiclass IC10Inst_FloatUnaArithClass<string opstr> {
  def r : IC10PseudoInst<(outs FPR:$dst), (ins FPR:$src),
                         opstr # " $dst $src">;
}

defm ABS : IC10Inst_FloatUnaArithClass<"abs">;
defm TRUNC : IC10Inst_FloatUnaArithClass<"trunc">;
defm ROUND : IC10Inst_FloatUnaArithClass<"round">;
defm FLOOR : IC10Inst_FloatUnaArithClass<"floor">;
defm CEIL : IC10Inst_FloatUnaArithClass<"ceil">;

defm SIN : IC10Inst_FloatUnaArithClass<"sin">;
defm COS : IC10Inst_FloatUnaArithClass<"cos">;
defm TAN : IC10Inst_FloatUnaArithClass<"tan">;
defm ASIN : IC10Inst_FloatUnaArithClass<"asin">;
defm ACOS : IC10Inst_FloatUnaArithClass<"acos">;
defm ATAN : IC10Inst_FloatUnaArithClass<"atan">;
defm SQRT : IC10Inst_FloatUnaArithClass<"sqrt">;
defm LOG : IC10Inst_FloatUnaArithClass<"log">;

defm NOT : IC10Inst_FloatUnaArithClass<"not">;

multiclass IC10Inst_FloatUnaArithPats<SDNode op, string inst> {
  def : Pat<(op FPR:$src), (!cast<Instruction>(inst # "r") FPR:$src)>;
}

defm : IC10Inst_FloatUnaArithPats<fabs, "ABS">;
defm : IC10Inst_FloatUnaArithPats<ftrunc, "TRUNC">;
defm : IC10Inst_FloatUnaArithPats<fround, "ROUND">;
defm : IC10Inst_FloatUnaArithPats<ffloor, "FLOOR">;
defm : IC10Inst_FloatUnaArithPats<fceil, "CEIL">;
defm : IC10Inst_FloatUnaArithPats<fsin, "SIN">;
defm : IC10Inst_FloatUnaArithPats<fcos, "COS">;
defm : IC10Inst_FloatUnaArithPats<ftan, "TAN">;
defm : IC10Inst_FloatUnaArithPats<fasin, "ASIN">;
defm : IC10Inst_FloatUnaArithPats<facos, "ACOS">;
defm : IC10Inst_FloatUnaArithPats<fatan, "ATAN">;
defm : IC10Inst_FloatUnaArithPats<fsqrt, "SQRT">;
defm : IC10Inst_FloatUnaArithPats<flog, "LOG">;

// select instructions

def SELECTrr : IC10PseudoInst<(outs FPR:$dst),
                              (ins GPR:$sel, FPR:$src1, FPR:$src2),
                              "select $dst $sel $src1 $src2">;
def SELECTri : IC10PseudoInst<(outs FPR:$dst),
                              (ins GPR:$sel, FPR:$src1, fimm64:$src2),
                              "select $dst $sel $src1 $src2">;
def SELECTir : IC10PseudoInst<(outs FPR:$dst),
                              (ins GPR:$sel, fimm64:$src1, FPR:$src2),
                              "select $dst $sel $src1 $src2">;
def SELECTii : IC10PseudoInst<(outs FPR:$dst),
                              (ins GPR:$sel, fimm64:$src1, fimm64:$src2),
                              "select $dst $sel $src1 $src2">;
def : Pat<(select GPR:$sel, FPR:$src1, FPR:$src2),
          (SELECTrr GPR:$sel, FPR:$src1, FPR:$src2)>;
def : Pat<(select GPR:$sel, FPR:$src1, fimm64:$src2),
          (SELECTri GPR:$sel, FPR:$src1, fimm64:$src2)>;
def : Pat<(select GPR:$sel, fimm64:$src1, FPR:$src2),
          (SELECTir GPR:$sel, fimm64:$src1, FPR:$src2)>;
def : Pat<(select GPR:$sel, fimm64:$src1, fimm64:$src2),
          (SELECTii GPR:$sel, fimm64:$src1, fimm64:$src2)>;

//===----------------------------------------------------------------------===//
// Conversions
//===----------------------------------------------------------------------===//

// force same source to promote optimization
let Constraints = "$src = $dst" in {
  def P_SINT2FP : IC10PseudoInst<(outs FPR:$dst), (ins GPR:$src),
                                "# pseudo sint2fp $dst $src"> {
    let Pattern = [(set FPR:$dst, (sint_to_fp GPR:$src))];
  }

  def P_UINT2FP : IC10PseudoInst<(outs FPR:$dst), (ins GPR:$src),
                                "# pseudo uint2fp $dst $src"> {
    let Pattern = [(set FPR:$dst, (uint_to_fp GPR:$src))];
  }
}

def P_FP2SINT : IC10PseudoInst<(outs GPR:$dst), (ins FPR:$src),
                               "# pseudo fp2sint $dst $src"> {
  let Pattern = [(set GPR:$dst, (fp_to_sint FPR:$src))];
}

def P_FP2UINT : IC10PseudoInst<(outs GPR:$dst), (ins FPR:$src),
                               "# pseudo fp2uint $dst $src"> {
  let Pattern = [(set GPR:$dst, (fp_to_uint FPR:$src))];
}

//===----------------------------------------------------------------------===//
// Compare & Set
//===----------------------------------------------------------------------===//

multiclass IC10Inst_IntCmpClass<string opstr> {
  def rr : IC10PseudoInst<(outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
                          "# int " # opstr # " $dst $src1 $src2">;
  def ri : IC10PseudoInst<(outs GPR:$dst), (ins GPR:$src1, uimm32:$imm2),
                          "# int " # opstr # " $dst $src1 $imm2">;
}

defm SEQI : IC10Inst_IntCmpClass<"seq">;
defm SNEI : IC10Inst_IntCmpClass<"sne">;
defm SGEI : IC10Inst_IntCmpClass<"sge">;
defm SGTI : IC10Inst_IntCmpClass<"sgt">;
defm SLEI : IC10Inst_IntCmpClass<"sle">;
defm SLTI : IC10Inst_IntCmpClass<"slt">;

multiclass IC10Inst_IntBrCCClass<string opstr> {
  let isBranch = 1, isTerminator = 1 in {
    def rr : IC10PseudoInst<(outs),
                            (ins GPR:$src1, GPR:$src2, BranchTarget:$tgt),
                            "# int " # opstr # " $src1 $src2 $tgt">;
    def ri : IC10PseudoInst<(outs),
                            (ins GPR:$src1, uimm32:$imm2, BranchTarget:$tgt),
                            "# int " # opstr # " $src1 $imm2 $tgt">;
  }
}

multiclass IC10Inst_FloatCmpClass<string opstr> {
  def rr: IC10Inst<(outs GPR:$dst), (ins FPR:$src1, FPR:$src2),
                   opstr # " $dst $src1 $src2">;
  def ri: IC10Inst<(outs GPR:$dst), (ins FPR:$src1, fimm64:$imm2),
                   opstr # " $dst $src1 $imm2">;
}

defm SEQ : IC10Inst_FloatCmpClass<"seq">;
defm SNE : IC10Inst_FloatCmpClass<"sne">;
defm SGE : IC10Inst_FloatCmpClass<"sge">;
defm SGT : IC10Inst_FloatCmpClass<"sgt">;
defm SLE : IC10Inst_FloatCmpClass<"sle">;
defm SLT : IC10Inst_FloatCmpClass<"slt">;

def SNANr: IC10Inst<(outs GPR:$dst), (ins FPR:$src1),
                    "snan $dst $src1">;
def SNANZr: IC10Inst<(outs GPR:$dst), (ins FPR:$src1),
                     "snanz $dst $src1">;

//===----------------------------------------------------------------------===//
// Compare & Branch
//===----------------------------------------------------------------------===//

defm BEQI : IC10Inst_IntBrCCClass<"beq">;
defm BNEI : IC10Inst_IntBrCCClass<"bne">;
defm BGEI : IC10Inst_IntBrCCClass<"bge">;
defm BGTI : IC10Inst_IntBrCCClass<"bgt">;
defm BLEI : IC10Inst_IntBrCCClass<"ble">;
defm BLTI : IC10Inst_IntBrCCClass<"blt">;

multiclass IC10Inst_FloatBrCCClass<string opstr> {
  let isBranch = 1, isTerminator = 1 in {
    def rr : IC10Inst<(outs), (ins FPR:$src1, FPR:$src2, BranchTarget:$tgt),
                      opstr # " $src1 $src2 $tgt">;
    def ri : IC10Inst<(outs), (ins FPR:$src1, fimm64:$imm2, BranchTarget:$tgt),
                      opstr # " $src1 $imm2 $tgt">;
  }
}

defm BEQ : IC10Inst_FloatBrCCClass<"beq">;
defm BNE : IC10Inst_FloatBrCCClass<"bne">;
defm BGE : IC10Inst_FloatBrCCClass<"bge">;
defm BGT : IC10Inst_FloatBrCCClass<"bgt">;
defm BLE : IC10Inst_FloatBrCCClass<"ble">;
defm BLT : IC10Inst_FloatBrCCClass<"blt">;

let isBranch = 1, isTerminator = 1 in
def BNANr : IC10Inst<(outs), (ins FPR:$src1, BranchTarget:$tgt),
                      "bnan $src1 $tgt">;

//===----------------------------------------------------------------------===//
// Load and Store Type Instructions
//===----------------------------------------------------------------------===//

// Load

def ILDi : IC10PseudoInst<(outs GPR:$dst), (ins uimm32:$imm),
                          "# load $dst at $imm">;
def ILDr : IC10PseudoInst<(outs GPR:$dst), (ins GPR:$addr),
                          "# load $dst at $addr">;

def : Pat<(load uimm32:$imm), (ILDi uimm32:$imm)>;
def : Pat<(load GPR:$addr), (ILDi GPR:$addr)>;

def LDi : IC10Inst<(outs FPR:$dst), (ins uimm32:$imm),
                   "get $dst d0 $imm">;
def LDr : IC10Inst<(outs FPR:$dst), (ins GPR:$addr),
                   "get $dst d0 $addr">;

def : Pat<(load uimm32:$imm), (LDi uimm32:$imm)>;
def : Pat<(load GPR:$addr), (LDi GPR:$addr)>;

// Store

def ISTrr : IC10PseudoInst<(outs), (ins GPR:$src1, GPR:$src2),
                           "# store $src1 at $src2">;
def ISTir : IC10PseudoInst<(outs), (ins uimm32:$imm1, GPR:$src2),
                           "# store $imm1 at $src2">;
def ISTri : IC10PseudoInst<(outs), (ins GPR:$src1, uimm32:$imm2),
                           "# store $src1 at $imm2">;
def ISTii : IC10PseudoInst<(outs), (ins uimm32:$imm1, uimm32:$imm2),
                           "# store $imm1 at $imm2">;

def : Pat<(store GPR:$src, GPR:$addr), (ISTrr GPR:$src, GPR:$addr)>;
def : Pat<(store uimm32:$src, GPR:$addr), (ISTir uimm32:$src, GPR:$addr)>;
def : Pat<(store GPR:$src, uimm32:$addr), (ISTri GPR:$src, uimm32:$addr)>;
def : Pat<(store uimm32:$src, uimm32:$addr), (ISTii uimm32:$src, uimm32:$addr)>;

def STrr : IC10Inst<(outs), (ins FPR:$src1, GPR:$src2),
                    "put db $src2 $src1">;
def STir : IC10Inst<(outs), (ins fimm64:$imm1, GPR:$src2),
                    "put db $src2 $imm1">;
def STri : IC10Inst<(outs), (ins FPR:$src1, uimm32:$imm2),
                    "put db $imm2 $src1">;
def STii : IC10Inst<(outs), (ins fimm64:$imm1, uimm32:$imm2),
                    "put db $imm2 $imm1">;

def : Pat<(store FPR:$src, GPR:$addr), (STrr FPR:$src, GPR:$addr)>;
def : Pat<(store fimm64:$src, GPR:$addr), (STir fimm64:$src, GPR:$addr)>;
def : Pat<(store FPR:$src, uimm32:$addr), (STri FPR:$src, uimm32:$addr)>;
def : Pat<(store fimm64:$src, uimm32:$addr), (STii fimm64:$src, uimm32:$addr)>;

let Uses = [SP] in {
  let Defs = [SP] in {
    let hasSideEffects = 1, mayStore = 1 in {
      def PUSHr : IC10Inst<(outs), (ins FPR:$src), "push $src">;
      def PUSHi : IC10Inst<(outs), (ins fimm64:$imm), "push $imm">;
    }
    let mayLoad = 1 in
    def POP : IC10Inst<(outs FPR:$dst), (ins), "pop $dst">;
  }
  let mayLoad = 1 in
  def PEEK : IC10Inst<(outs FPR:$dst), (ins), "peek $dst">;
}

let Uses = [SP] in {
  let Defs = [SP] in {
    let hasSideEffects = 1, mayStore = 1 in {
      def IPUSHr : IC10PseudoInst<(outs), (ins GPR:$src), "#int push $src">;
      def IPUSHi : IC10PseudoInst<(outs), (ins uimm32:$imm), "#int push $imm">;
    }
    let mayLoad = 1 in
    def IPOP : IC10PseudoInst<(outs GPR:$dst), (ins), "#int pop $dst">;
  }
  let mayLoad = 1 in
  def IPEEK : IC10PseudoInst<(outs GPR:$dst), (ins), "#int peek $dst">;
}

//===----------------------------------------------------------------------===//
// Special Instructions (BR, Call, Ret, Stack)
//===----------------------------------------------------------------------===//

let isTerminator = 1, isBranch = 1, isBarrier = 1 in
def Ji : IC10Inst<(outs), (ins uimm32:$tgt), "j $tgt">;

let isTerminator = 1, isBranch = 1, isBarrier = 1 in
def Jr : IC10Inst<(outs), (ins GPR:$tgt), "j $tgt">;

let Uses = [R15], Defs = [RA], isCall = 1 in
def JALi : IC10Inst<(outs), (ins uimm32:$tgt), "jal $tgt">;

let Uses = [R15], Defs = [RA], isCall = 1 in
def CALL_PSEUDO : IC10PseudoInst<(outs), (ins uimm32:$tgt), "# call $tgt">;

let Uses = [R15, RA], isTerminator = 1, isReturn = 1, isBarrier = 1 in
def RET_PSEUDO : IC10PseudoInst<(outs), (ins variable_ops), "# ret">;

let Defs = [R15], Uses = [R15] in {
  def ADJCALLSTACKDOWN : IC10PseudoInst<
      (outs), (ins uimm32:$amt, uimm32:$amt2),
      "# ADJCALLSTACKDOWN">;

  def ADJCALLSTACKUP : IC10PseudoInst<
      (outs), (ins uimm32:$amt1, uimm32:$amt2),
      "# ADJCALLSTACKUP">;
}

//===----------------------------------------------------------------------===//
// Non-Instruction Patterns
//===----------------------------------------------------------------------===//

// Target Global Address
// def : Pat<(i32 (IC10Wrapper tglobaladdr:$dst)), (MOVE_I_I tglobaladdr:$dst)>;

// Calls
// def : Pat<(IC10Call (i32 tglobaladdr:$addr)), (CALL_PSEUDO tglobaladdr:$addr)>;

//===----------------------------------------------------------------------===//
// Intrinsics
//===----------------------------------------------------------------------===//

let hasSideEffects = 1 in {
  def YIELD : IC10Inst<(outs), (ins), "yield">;
  def SLEEPr : IC10Inst<(outs), (ins FPR:$src), "sleep $src">;
  def SLEEPi : IC10Inst<(outs), (ins fimm64:$imm), "sleep $imm">;
  let isTrap = 1, isTerminator = 1, isBarrier = 1 in
  def HCF : IC10Inst<(outs), (ins), "hcf">;
}

def : Pat<(int_ic10_yield), (YIELD)>;
def : Pat<(int_ic10_sleep FPR:$src), (SLEEPr FPR:$src)>;
def : Pat<(int_ic10_sleep fimm64:$imm), (SLEEPi fimm64:$imm)>;
def : Pat<(trap), (HCF)>;
