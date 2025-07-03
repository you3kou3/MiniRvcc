//===-- MiniRvcc.td - Describe the MiniRvcc Target Machine ------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the MiniRvcc target for the LLVM target-independent code
// generator.
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"



//===----------------------------------------------------------------------===//
// MiniRvcc processors supported.
//===----------------------------------------------------------------------===//



//===----------------------------------------------------------------------===//
// Registers, calling conventions, instruction descriptions.
//===----------------------------------------------------------------------===//

include "MiniRvccRegisterInfo.td"
include "MiniRvccInstrInfo.td"


//===----------------------------------------------------------------------===//
// Define the MiniRvcc target.
//===----------------------------------------------------------------------===//

def MiniRvccInstrInfo : InstrInfo;

def MiniRvcc : Target {
   let InstructionSet = MiniRvccInstrInfo;
}
