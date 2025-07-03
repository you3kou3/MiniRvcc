//===-- MiniRvccInstrInfo.td - Target Description for MiniRvcc --*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the MiniRvcc instructions in TableGen format.
//
// This target is for research and self-learning purposes, extending a RISC-V
// base with additional pseudo-instructions including MAC (Multiply-Accumulate)
// operations.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// MiniRvcc specific DAG Nodes.
//===----------------------------------------------------------------------===//

include "MiniRvccInstrFormats.td"
