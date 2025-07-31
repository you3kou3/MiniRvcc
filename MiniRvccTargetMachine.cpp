//===-- MiniRvccTargetMachine.cpp - Define TargetMachine for MiniRvcc ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implements the info about MiniRvcc target spec.
//
//===----------------------------------------------------------------------===//

#include "MiniRvcc.h"
#include "MiniRvccTargetMachine.h"                     // 自作ターゲットマシン定義
#include "MiniRvccSubtarget.h"                         // 自作サブターゲット定義
#include "llvm/CodeGen/TargetPassConfig.h"             // createPassConfig に必要
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h" // ELF ファイル出力関連
#include "llvm/MC/TargetRegistry.h"                    // LLVMInitializeXXXTarget 関数など
#include "llvm/Target/TargetOptions.h"                 // TargetOptions のため
#include "llvm/Target/TargetMachine.h"                 // ベースクラス定義（通常は .h に含めてOK）
#include "llvm/Target/TargetSubtargetInfo.h"           // Subtarget を直接使うなら
#include "llvm/Analysis/TargetTransformInfo.h"         // getTargetTransformInfo() を使うなら


using namespace llvm;


extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMiniRvccTarget() {
    RegisterTargetMachine<MiniRvccTargetMachine> X(getTheMiniRvccTarget());
}


static StringRef computeDataLayout(const Triple &TT) {
    return "e-m:e-p:32:32-i64:64-n32-S128";
}

static Reloc::Model getEffectiveRelocModel(const Triple &TT,
                                           Optional<Reloc::Model> RM) {
  return RM.value_or(Reloc::Static);
}


MiniRvccTargetMachine::MiniRvccTargetMachine(const Target &T, const Triple &TT,
                                             StringRef CPU, StringRef FS,
                                             const TargetOptions &Options,
                                             Optional<Reloc::Model> RM,
                                             Optional<CodeModel::Model> CM,
                                             CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(T, computeDataLayout(TT), TT, CPU, FS, Options,
                        getEffectiveRelocModel(TT, RM),
                        getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<MiniRvccELFTargetObjectFile>()) 
{
    initAsmInfo();
    //setMachineOutliner(true);
    //setSupportsDefaultOutlining(true);
}


const MiniRvccSubtarget *
MiniRvccTargetMachine::getSubtargetImpl(const Function &F) const 
{
    static auto ST = std::make_unique<MiniRvccSubtarget>(
        TargetTriple, "generic", "", *this);
    return ST.get(); // 毎回同じ固定インスタンスを返す
}


TargetPassConfig *MiniRvccTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new MiniRvccPassConfig(*this, PM);
}


TargetTransformInfo
MiniRvccTargetMachine::getTargetTransformInfo(const Function &F) const {
  return TargetTransformInfo(MiniRvccTTIImpl(this, F));
}


// A RISC-V hart has a single byte-addressable address space of 2^XLEN bytes
// for all memory accesses, so it is reasonable to assume that an
// implementation has no-op address space casts. If an implementation makes a
// change to this, they can override it here.
bool MiniRvccTargetMachine::isNoopAddrSpaceCast(unsigned SrcAS,
                                                unsigned DstAS) const {
  return true;
}

