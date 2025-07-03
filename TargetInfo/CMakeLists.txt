add_llvm_component_library(LLVMMiniRvccInfo
  MiniRvccTargetInfo.cpp
  
  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  MiniRvcc
)
