// IC10

// General register definition
class IC10Reg<bits<16> Enc, string n> : Register<n> {
  let Namespace = "IC10";
  let HWEncoding = Enc;
}

// General purpose registers
def R0 : IC10Reg<0, "r0">;
def R1 : IC10Reg<1, "r1">;
def R2 : IC10Reg<2, "r2">;
def R3 : IC10Reg<3, "r3">;
def R4 : IC10Reg<4, "r4">;
def R5 : IC10Reg<5, "r5">;
def R6 : IC10Reg<6, "r6">;
def R7 : IC10Reg<7, "r7">;
def R8 : IC10Reg<8, "r8">;
def R9 : IC10Reg<9, "r9">;
def R10 : IC10Reg<10, "r10">;
def R11 : IC10Reg<11, "r11">;
def R12 : IC10Reg<12, "r12">;
def R13 : IC10Reg<13, "r13">;
def R14 : IC10Reg<14, "r14">;
def R15 : IC10Reg<15, "r15">;
def SP : IC10Reg<16, "sp">;
def RA : IC10Reg<17, "ra">;

def GPR : RegisterClass<"IC10", [i64], 64, (add (sequence "R%u", 0, 15), SP, RA)>;
def FPR : RegisterClass<"IC10", [f64], 64, (add (sequence "R%u", 0, 15), SP, RA)>;
