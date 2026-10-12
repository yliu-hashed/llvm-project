add_llvm_component_library(LLVMIC10Info
  IC10TargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  IC10
  )
