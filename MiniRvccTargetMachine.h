//===-- MiniRvccTargetMachine.h - Define TargetMachine for MiniRvcc --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the TargetMachine class for the MiniRvcc target.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MINIRVCC_MINIRVCCTARGETMACHINE_H
#define LLVM_LIB_TARGET_MINIRVCC_MINIRVCCTARGETMACHINE_H

#include "llvm/Target/TargetMachine.h"
#include "llvm/IR/DataLayout.h"

namespace llvm {
class MiniRvccTargetMachine : public LLVMTargetMachine {
public:
    MiniRvccTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                          StringRef FS, const TargetOptions &Options,
                          Optional<Reloc::Model> RM,
                          Optional<CodeModel::Model> CM,
                          CodeGenOpt::Level OL, bool JIT);
    ~MiniRvccTargetMachine() override;
};
} // namespace llvm

#endif
