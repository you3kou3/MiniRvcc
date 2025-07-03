
add_llvm_component_group(MiniRvcc)
set(LLVM_TARGET_DEFINITIONS MiniRvcc.td)

tablegen(LLVM MiniRvccGenRegisterInfo.inc -gen-register-info)

add_public_tablegen_target(MiniRvccCommonTableGen)

add_llvm_target(MiniRvccCodeGen
  MiniRvccTargetMachine.cpp
)

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
