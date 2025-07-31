//=- MiniRvccMachineFunctionInfo.cpp - MiniRvcc machine function info ---*- C++ -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares MiniRvcc-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//

#include "MiniRvccMachineFunctionInfo.h"

using namespace llvm;

// MiniRvccMachineFunctionInfo のコピーコンストラクタ
// RV32Iでは varargs（可変長引数）をサポートしないため、ダミー値で初期化する
yaml::MiniRvccMachineFunctionInfo::MiniRvccMachineFunctionInfo(
    const llvm::MiniRvccMachineFunctionInfo &MFI)
    : VarArgsFrameIndex(-1/*MFI.getVarArgsFrameIndex()*/), // 無効なフレームインデックス（使用しない）
      VarArgsSaveSize(0/*MFI.getVarArgsSaveSize()*/) {}    // 可変引数用の保存サイズ（未使用） {}


/// MachineFunctionInfo を別の関数に複製するときに呼ばれる.
/// たとえば関数のインライン展開時などに必要。
MachineFunctionInfo *MiniRvccMachineFunctionInfo::clone(
    BumpPtrAllocator &Allocator, MachineFunction &DestMF,
    const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB) const 
{
    return DestMF.cloneInfo<MiniRvccMachineFunctionInfo>(*this);
}


/// YAML 出力や読み込みのときに呼ばれる関数
/// MiniRvccMachineFunctionInfo を YAML にマップするための処理
/// 通常はデバッグ用やテスト用の目的で使用される
void yaml::MiniRvccMachineFunctionInfo::mappingImpl(yaml::IO &YamlIO) {
    // MappingTraits<MiniRvccMachineFunctionInfo> に定義された mapping() を呼び出す
    MappingTraits<MiniRvccMachineFunctionInfo>::mapping(YamlIO, *this);
}

/// YAMLから読み込んだMiniRvccMachineFunctionInfo (YamlMFI) を使って、
/// このMiniRvccMachineFunctionInfoの基本フィールドを初期化する。
/// RV32I固有のフィールド（例: RV32StackSize）があればここでも設定する。
void MiniRvccMachineFunctionInfo::initializeBaseYamlFields(
    const yaml::MiniRvccMachineFunctionInfo &YamlMFI) {
    // 可変長引数のフレームインデックスを設定
    VarArgsFrameIndex = YamlMFI.VarArgsFrameIndex;

    // 可変長引数の保存サイズを設定
    VarArgsSaveSize = YamlMFI.VarArgsSaveSize;
}
