add_llvm_component_library(LLVMIC10Desc
  IC10InstPrinter.cpp
  IC10MCAsmInfo.cpp
  IC10MCTargetDesc.cpp

  LINK_COMPONENTS
  MC
  IC10Info
  Support

  ADD_TO_COMPONENT
  IC10
  )
