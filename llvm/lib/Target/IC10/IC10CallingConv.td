// IC10

// Return Value Calling Convention
def RetCC_IC10 : CallingConv<[
  CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,
  CCIfType<[i64, f64], CCAssignToReg<[R0, R1]>>,

  // Stack
  CCIfType<[i32], CCAssignToStack<4, 8>>,
  CCIfType<[i16], CCAssignToStack<2, 8>>,
  CCIfType<[i8 ], CCAssignToStack<1, 8>>
]>;

// IC10 Argument Calling Conventions
def CC_IC10 : CallingConv<[
  // Pass by value if the byval attribute is given
  CCIfByVal<CCPassByVal<8, 8>>,

  CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,
  CCIfType<[i64, f64], CCAssignToReg<[R0, R1, R2, R3]>>,

  // Stack
  CCIfType<[f64], CCAssignToStack<8, 8>>,
  CCIfType<[i64], CCAssignToStack<8, 8>>,
]>;

// Callee doesn't promise to save any registers.
def CC_Save : CalleeSavedRegs<(add (sequence "R%u", 8, 15), RA)>;
