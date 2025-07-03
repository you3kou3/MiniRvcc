//===-- MiniRvccRegisterInfo.td - MiniRvcc Register defs --------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Declarations that describe the MiniRvcc register files.
//
// This target is based on RISC-V for self-learning and research purposes,
// and includes extensions such as pseudo-instructions for MAC operations.
//
//===----------------------------------------------------------------------===//

let Namespace = "MiniRvcc" in {
class MiniRvccReg<bits<16> Enc, string n, list<string> alt = []> : Register<n> {
  let HWEncoding = Enc;
  let AltNames = alt;
}
}

// MiniRvcc CPU Registers
class MiniRvccGPRReg<bits<16> Enc, string n> : MiniRvccReg<Enc, n>;


let Namespace = "MiniRvcc" in {
  def ZERO : MiniRvccReg<0,  "zero">, DwarfRegNum<[0]>;
}

def CPURegs : RegisterClass<"MiniRvcc", [i32], 32, (add ZERO)>;
