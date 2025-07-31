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

#include "MCTargetDesc/MiniRvccMCTargetDesc.h"
#include "MiniRvccSubtarget.h"
#include "llvm/CodeGen/SelectionDAGTargetInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class MiniRvccTargetMachine : public LLVMTargetMachine {
    std::unique_ptr<TargetLoweringObjectFile> TLOF;
    mutable StringMap<std::unique_ptr<MiniRvccSubtarget>> SubtargetMap;

public:
    MiniRvccTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                          StringRef FS, const TargetOptions &Options,
                          Optional<Reloc::Model> RM,
                          Optional<CodeModel::Model> CM,
                          CodeGenOpt::Level OL, bool JIT);
    //~MiniRvccTargetMachine() override;

    const MiniRvccSubtarget *getSubtargetImpl(const Function &F) const override;
    const MiniRvccSubtarget *getSubtargetImpl() const = delete;
    TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
    TargetLoweringObjectFile *getObjFileLowering() const override {
        return TLOF.get();
    }
    TargetTransformInfo getTargetTransformInfo(const Function &F) const override;
    bool isNoopAddrSpaceCast(unsigned SrcAS, unsigned DstAS) const override;
};
} // namespace llvm

#endif
