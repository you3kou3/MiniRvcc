
add_llvm_component_group(MiniRvcc)
set(LLVM_TARGET_DEFINITIONS MiniRvcc.td)

tablegen(LLVM MiniRvccGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM MiniRvccGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM MiniRvccGenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM MiniRvccGenMCPseudoLowering.inc -gen-pseudo-lowering)
tablegen(LLVM MiniRvccGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM MiniRvccGenCallingConv.inc -gen-callingconv)

tablegen(LLVM MiniRvccGenCodeEmitter.inc -gen-emitter)
tablegen(LLVM MiniRvccGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM MiniRvccGenAsmWriter.inc -gen-asm-writer)

add_public_tablegen_target(MiniRvccCommonTableGen)


add_llvm_target(MiniRvccCodeGen
  MiniRvccTargetMachine.cpp
  MiniRvccSubtarget.cpp
  MiniRvccRegisterInfo.cpp
  MiniRvccFrameLowering.cpp
  MiniRvccInstrInfo.cpp
  MiniRvccISelLowering.cpp
  MiniRvccMachineFunction.cpp
  MiniRvccTargetObjectFile.cpp
  MiniRvccAsmPrinter.cpp
  MiniRvccMCInstrLower.cpp
  MiniRvccISelDAGToDAG.cpp
)

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
