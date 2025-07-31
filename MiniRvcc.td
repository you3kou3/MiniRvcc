//===-- MiniRvcc.td - Describe the MiniRvcc Target Machine -*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Registers, calling conventions, instruction descriptions.
//===----------------------------------------------------------------------===//

include "MiniRvccSchedule.td"
include "MiniRvccRegisterInfo.td"
include "MiniRvccCallingConv.td"
include "MiniRvccInstrInfo.td"


//===----------------------------------------------------------------------===//
// MiniRvcc processors supported.
//===----------------------------------------------------------------------===//

def : ProcessorModel<"generic-rv32", NoSchedModel, []>;


//===----------------------------------------------------------------------===//
// Define the MiniRvcc target.
//===----------------------------------------------------------------------===//

def MiniRvccInstrInfo : InstrInfo {
    let guessInstructionProperties = 0;
}


def MiniRvccAsmParser : AsmParser {
  let ShouldEmitMatchRegisterAltName = 1;
  let AllowDuplicateRegisterNames = 1;
}

def MiniRvccAsmWriter : AsmWriter {
  int PassSubtarget = 1;
}

def MiniRvcc : Target {
  let InstructionSet = MiniRvccInstrInfo;
  let AssemblyParsers = [MiniRvccAsmParser];
  let AssemblyWriters = [MiniRvccAsmWriter];
  let AllowRegisterRenaming = 1;
}
