add_llvm_component_group(IC10)

set(LLVM_TARGET_DEFINITIONS IC10.td)

tablegen(LLVM IC10GenGlobalISel.inc       -gen-global-isel)
tablegen(LLVM IC10GenRegisterInfo.inc     -gen-register-info)
tablegen(LLVM IC10GenRegisterBank.inc     -gen-register-bank)
tablegen(LLVM IC10GenInstrInfo.inc        -gen-instr-info)
tablegen(LLVM IC10GenSubtargetInfo.inc    -gen-subtarget)
tablegen(LLVM IC10GenCallingConv.inc      -gen-callingconv)
tablegen(LLVM IC10GenAsmWriter.inc        -gen-asm-writer)

tablegen(LLVM IC10GenPostLegalizeGICombiner.inc -gen-global-isel-combiner
              -combiners="IC10PostLegalizerCombiner")
tablegen(LLVM IC10GenPreLegalizeGICombiner.inc -gen-global-isel-combiner
              -combiners="IC10PreLegalizerCombiner")

add_public_tablegen_target(IC10CommonTableGen)

add_llvm_target(IC10CodeGen
  GISel/IC10CallLowering.cpp
  GISel/IC10InstructionSelector.cpp
  GISel/IC10LegalizerInfo.cpp
  GISel/IC10RegisterBankInfo.cpp
  GISel/IC10AddressabilityFixPass.cpp
  GISel/IC10PreLegalizerCombiner.cpp
  GISel/IC10PostLegalizerCombiner.cpp
  GISel/IC10ExpandPseudoInsts.cpp
  GISel/IC10LateCombinePushPopPass.cpp

  IC10AsmPrinter.cpp
  IC10InstrInfo.cpp
  IC10FrameLowering.cpp
  IC10MachineFunctionInfo.cpp
  IC10RegisterInfo.cpp
  IC10Subtarget.cpp
  IC10TargetMachine.cpp
  IC10TargetLowering.cpp
  IC10MCInstLower.cpp
  IC10CodeGenPassBuilder.cpp
  IC10TargetObjectFile.cpp

  LINK_COMPONENTS
  AsmPrinter
  CodeGen
  CodeGenTypes
  Core
  MC
  GlobalISel
  IC10Desc
  IC10Info
  Support
  Target
  Passes

  ADD_TO_COMPONENT
  IC10
  )

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
