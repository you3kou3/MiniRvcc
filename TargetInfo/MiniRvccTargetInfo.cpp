//===-- MiniRvccTargetInfo.cpp - MiniRvcc Target Implementation ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// This file implements target information for the MiniRvcc target.
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/MiniRvccTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
using namespace llvm;


Target &llvm::getTheMiniRvccTarget() {
    static Target TheMiniRvccTarget;
    return TheMiniRvccTarget;
}


extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMiniRvccTargetInfo() {
    RegisterTarget<Triple::minirvcc, /*HasJIT=*/true> X(
        getTheMiniRvccTarget(), "minirvcc", "32-bit Mini RV32", "Mini RISC-V for learn_simple_soc");
}

