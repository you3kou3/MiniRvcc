//===-- MiniRvccMCTargetDesc.h - MiniRvcc Target Descriptions --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides MiniRvcc-specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MINIRVCC_MCTARGETDESC_MINIRVCCMCTARGETDESC_H
#define LLVM_LIB_TARGET_MINIRVCC_MCTARGETDESC_MINIRVCCMCTARGETDESC_H

#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/DataTypes.h"
#include <memory>

namespace llvm {
class Target;
extern Target &getTheMiniRvccTarget();
}


#endif

