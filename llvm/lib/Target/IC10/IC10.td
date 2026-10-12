// IC10

include "llvm/Target/Target.td"

include "IC10RegisterInfo.td"
include "GISel/IC10RegisterBanks.td"
include "IC10CallingConv.td"
include "IC10InstrInfo.td"
include "IC10Combines.td"

def IC10InstrInfo : InstrInfo;

def : Processor<"generic", NoItineraries, []>;

def IC10AsmWriter : AsmWriter {
  string AsmWriterClassName  = "InstPrinter";
  int PassSubtarget = 1;
  int Variant = 0;
}

defm : RemapAllTargetPseudoPointerOperands<GPR>;

def IC10 : Target {
  let InstructionSet = IC10InstrInfo;
  let AssemblyWriters = [IC10AsmWriter];
}
