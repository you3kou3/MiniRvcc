//===-- MiniRvccISelDAGToDAG.cpp - A dag to dag inst selector for MiniRvcc ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the MiniRvcc target.
//
//===----------------------------------------------------------------------===//

#include "MiniRvccISelDAGToDAG.h"
#include "MCTargetDesc/MiniRvccMCTargetDesc.h"
#include "MCTargetDesc/MiniRvccMatInt.h"
#include "MiniRvccISelLowering.h"
#include "MiniRvccMachineFunctionInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/IR/IntrinsicsMiniRvcc.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "minirvcc-isel"

namespace llvm {
namespace MiniRvcc {
#define GET_RISCVVSSEGTable_IMPL
#define GET_RISCVVLSEGTable_IMPL
#define GET_RISCVVLXSEGTable_IMPL
#define GET_RISCVVSXSEGTable_IMPL
#define GET_RISCVVLETable_IMPL
#define GET_RISCVVSETable_IMPL
#define GET_RISCVVLXTable_IMPL
#define GET_RISCVVSXTable_IMPL
#define GET_RISCVMaskedPseudosTable_IMPL
#include "RISCVGenSearchableTables.inc"
} // namespace RISCV
} // namespace llvm



/// SelectionDAG に即値展開命令列を挿入し、最終命令ノードを返す関数
static SDNode *selectImmSeq(SelectionDAG *CurDAG, 
                            const SDLoc &DL, 
                            const MVT VT,
                            MiniRvccMatInt::InstSeq &Seq) 
{
    SDNode *Result = nullptr;

    // 最初の命令のソースレジスタは常に X0（ゼロレジスタ）を使用
    SDValue SrcReg = CurDAG->getRegister(MiniRvcc::X0, VT);

    // 即値ロード命令列（Seq）を順に処理
    for (MiniRvccMatInt::Inst &Inst : Seq) {
        // 即値オペランドを DAG 上の定数ノードとして生成
        SDValue SDImm = CurDAG->getTargetConstant(Inst.Imm, DL, VT);

        // オペランドの種類ごとに異なる MachineNode を生成
        switch (Inst.getOpndKind()) {
        case MiniRvccMatInt::Imm:
            // 即値単体を取る命令（例：LUI）
            Result = CurDAG->getMachineNode(Inst.Opc, // ターゲット命令の Opcode
                                            DL,       // デバッグ情報や位置情報（命令のソースコード位置など）
                                            VT,       // 値の型（例: MVT::i32）
                                            SDImm);   // オペランド（引数）を SDValue 形式で渡す
            break;
        case MiniRvccMatInt::RegX0:
            // ソースに X0（ゼロレジスタ）を使う命令（例：ADDI xN, x0, imm）
            Result = CurDAG->getMachineNode(Inst.Opc, 
                                            DL, 
                                            VT, 
                                            SrcReg,
                                            CurDAG->getRegister(MiniRvcc::X0, VT));
            break;
        case MiniRvccMatInt::RegReg:
            // ソースに2つのレジスタ（同じ値）を使う命令（例：ADD xN, xN, xN）
            Result = CurDAG->getMachineNode(Inst.Opc, 
                                            DL, 
                                            VT, 
                                            SrcReg, 
                                            SrcReg);
            break;
        case MiniRvccMatInt::RegImm:
            // レジスタと即値を使う命令（例：ADDI xN, xN, imm）
            Result = CurDAG->getMachineNode(Inst.Opc, 
                                            DL, 
                                            VT, 
                                            SrcReg, 
                                            SDImm);
            break;
        }

        // Only the first instruction has X0 as its source.
        // 次の命令のソースレジスタは直前命令の出力を使うように更新
        SrcReg = SDValue(Result, 0);
    }

    return Result;
}


/// 即値 Imm を LUI / ADDI 命令のシーケンスに変換し、SelectionDAG にノードを追加する。
static SDNode *selectImm(SelectionDAG *CurDAG, 
                         const SDLoc &DL, 
                         const MVT VT,
                         int64_t Imm, 
                         const MiniRvccSubtarget &Subtarget) 
{
    // 即値をロードする命令列（LUI / ADDIなど）を生成（RV32I機能ビットで制限）
    MiniRvccMatInt::InstSeq Seq =
        MiniRvccMatInt::generateInstSeq(Imm, Subtarget.getFeatureBits());

    // 命令列を SelectionDAG ノードに変換
    return selectImmSeq(CurDAG, DL, VT, Seq);
}



void MiniRvccDAGToDAGISel::Select(SDNode *Node) {
    // すでにマシン命令ノードなら選択済みなので何もしない
    if (Node->isMachineOpcode()) {
        LLVM_DEBUG(dbgs() << "== "; Node->dump(CurDAG); dbgs() << "\n");
        Node->setNodeId(-1);
        return;
    }

    unsigned Opcode = Node->getOpcode();

    // RV32IならXLenは32bitなので i32 型
    MVT XLenVT = Subtarget->getXLenVT();
    SDLoc DL(Node);
    MVT VT = Node->getSimpleValueType(0);


    // RV32Iでは32ビットの即値を1命令（ADDIなど）で直接扱えない場合が多いため、
    // 32ビット定数は通常、LUI命令（上位20ビットをロード）とADDI命令（下位12ビットを加算）
    // などの複数命令の組み合わせで表現されるため、以下のコードで対応する
    switch (Opcode) {
    case ISD::Constant: {
        // 定数ノードをキャスト
        auto *ConstNode = cast<ConstantSDNode>(Node);
        // 定数が0で、型が32bit (RV32I)ならゼロレジスタX0からのコピーに置き換え
        if (VT == XLenVT && ConstNode->isZero()) {
            SDValue New =
                CurDAG->getCopyFromReg(CurDAG->getEntryNode(), DL, RISCV::X0, XLenVT);
            ReplaceNode(Node, New.getNode());
            return;
        }

        // 即値を符号拡張した値として取得
        int64_t Imm = ConstNode->getSExtValue();

        // RV32Iでは16bit即値や圧縮命令は標準セットではないためコメントアウト
        /*
        if (isUInt<16>(Imm) && isInt<12>(SignExtend64<16>(Imm)) && hasAllHUsers(Node))
            Imm = SignExtend64<16>(Imm);
        */

        // 32bit符号拡張の判定は残しておく（RV32Iでのsimm32チェック）
        if (!isInt<32>(Imm) && isUInt<32>(Imm) /*&& hasAllWUsers(Node)*/)
            Imm = SignExtend64<32>(Imm);

        // RV32I向けの即値選択処理に委譲
        ReplaceNode(Node, selectImm(CurDAG, DL, VT, Imm, *Subtarget));
        return;
    }

    // それ以外はTableGen自動生成の選択ルールに任せる
    SelectCode(Node);
}

bool MiniRvccDAGToDAGISel::SelectInlineAsmMemoryOperand(
    const SDValue &Op, unsigned ConstraintID, std::vector<SDValue> &OutOps) 
{
    switch (ConstraintID) {
    case InlineAsm::Constraint_m:
        // We just support simple memory operands that have a single address
        // operand and need no special handling.
        OutOps.push_back(Op);
        return false;
    case InlineAsm::Constraint_A:
        OutOps.push_back(Op);
        return false;
    default:
        break;
    }

    return true;
}


bool MiniRvccDAGToDAGISel::SelectAddrFrameIndex(SDValue Addr, SDValue &Base,
                                             SDValue &Offset) 
{
    if (auto *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
        Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), Subtarget->getXLenVT());
        Offset = CurDAG->getTargetConstant(0, SDLoc(Addr), Subtarget->getXLenVT());
        return true;
    }

    return false;
}

// Select a frame index and an optional immediate offset from an ADD or OR.
bool MiniRvccDAGToDAGISel::SelectFrameAddrRegImm(SDValue Addr, SDValue &Base,
                                              SDValue &Offset) 
{
    // フレームインデックス（FI）そのものの形式なら、SelectAddrFrameIndex()で処理
    if (SelectAddrFrameIndex(Addr, Base, Offset))
        return true;

    // Base + オフセットの形式でないならfalseを返す（=サポート外）
    if (!CurDAG->isBaseWithConstantOffset(Addr))
        return false;

    // BaseがFrameIndexなら処理を続ける（スタック変数など）
    if (auto *FIN = dyn_cast<FrameIndexSDNode>(Addr.getOperand(0))) {
        // オフセット値を定数ノードから取得
        int64_t CVal = cast<ConstantSDNode>(Addr.getOperand(1))->getSExtValue();

        // RV32Iの命令フォーマット（12bit即値）に収まる場合だけ処理
        if (isInt<12>(CVal)) {
            // BaseはターゲットFrameIndex
            Base = CurDAG->getTargetFrameIndex(FIN->getIndex(),
                                               Subtarget->getXLenVT());
            // Offsetはターゲット定数
            Offset = CurDAG->getTargetConstant(CVal, SDLoc(Addr),
                                               Subtarget->getXLenVT());
            return true;
        }
    }

    return false;
}

// Fold constant addresses.
static bool selectConstantAddr(SelectionDAG *CurDAG, const SDLoc &DL,
                               const MVT VT, const MiniRvccSubtarget *Subtarget,
                               SDValue Addr, SDValue &Base, SDValue &Offset) 
{
    if (!isa<ConstantSDNode>(Addr))
        return false;

    int64_t CVal = cast<ConstantSDNode>(Addr)->getSExtValue();

    // RV32Iでは、simm12内に収まる定数だけを扱う。
    if (isInt<12>(CVal)) {
        Base = CurDAG->getRegister(MiniRvcc::X0, VT);
        Offset = CurDAG->getTargetConstant(CVal, DL, VT);
        return true;
    }

    return false; 

    /*
    // If the constant is a simm12, we can fold the whole constant and use X0 as
    // the base. If the constant can be materialized with LUI+simm12, use LUI as
    // the base. We can't use generateInstSeq because it favors LUI+ADDIW.
    int64_t Lo12 = SignExtend64<12>(CVal);
    int64_t Hi = (uint64_t)CVal - (uint64_t)Lo12;
    if (!Subtarget->is64Bit() || isInt<32>(Hi)) {
        if (Hi) {
            int64_t Hi20 = (Hi >> 12) & 0xfffff;
            Base = SDValue(CurDAG->getMachineNode(MiniRvcc::LUI, DL, VT,
                           CurDAG->getTargetConstant(Hi20, DL, VT)),
                           0);
        } else {
            Base = CurDAG->getRegister(RISCV::X0, VT);
        }
        Offset = CurDAG->getTargetConstant(Lo12, DL, VT);
        return true;
    }

    // Ask how constant materialization would handle this constant.
    MiniRvccMatInt::InstSeq Seq =
        MiniRvccMatInt::generateInstSeq(CVal, Subtarget->getFeatureBits());

    // If the last instruction would be an ADDI, we can fold its immediate and
    // emit the rest of the sequence as the base.
    if (Seq.back().Opc != MiniRvcc::ADDI)
        return false;
    Lo12 = Seq.back().Imm;

    // Drop the last instruction.
    Seq.pop_back();
    assert(!Seq.empty() && "Expected more instructions in sequence");

    Base = SDValue(selectImmSeq(CurDAG, DL, VT, Seq), 0);
    Offset = CurDAG->getTargetConstant(Lo12, DL, VT);
    return true;
    */
}

// Is this ADD instruction only used as the base pointer of scalar loads and
// stores?
static bool isWorthFoldingAdd(SDValue Add) 
{
    for (auto Use : Add->uses()) {
        unsigned Opcode = Use->getOpcode();
        
        // RV32IではLOADとSTOREだけを対象にする
        if (Opcode != ISD::LOAD && Opcode != ISD::STORE)
            return false;

        EVT VT = cast<MemSDNode>(Use)->getMemoryVT();

        // スカラ整数型である必要がある（f32など浮動小数点は除外）
        if (!VT.isScalarInteger())
            return false;

        // STOREでアドレスとしてでなく、値としてAddが使われている場合はNG
        if (Opcode == ISD::STORE &&
            cast<StoreSDNode>(Use)->getValue() == Add)
            return false;
    }

    return true;

    /*
    for (auto Use : Add->uses()) {
        if (Use->getOpcode() != ISD::LOAD && Use->getOpcode() != ISD::STORE &&
            Use->getOpcode() != ISD::ATOMIC_LOAD &&
            Use->getOpcode() != ISD::ATOMIC_STORE)
            return false;
        EVT VT = cast<MemSDNode>(Use)->getMemoryVT();
        if (!VT.isScalarInteger() && VT != MVT::f16 && VT != MVT::f32 &&
            VT != MVT::f64)
            return false;
        // Don't allow stores of the value. It must be used as the address.
        if (Use->getOpcode() == ISD::STORE &&
            cast<StoreSDNode>(Use)->getValue() == Add)
            return false;
        if (Use->getOpcode() == ISD::ATOMIC_STORE &&
            cast<AtomicSDNode>(Use)->getVal() == Add)
            return false;
    }

    return true;
    */
}

bool MiniRvccDAGToDAGISel::SelectAddrRegImm(SDValue Addr, SDValue &Base,
                                         SDValue &Offset) 
{
    // フレームインデックスからのアクセスに対応
    if (SelectAddrFrameIndex(Addr, Base, Offset))
        return true;

    SDLoc DL(Addr);
    MVT VT = Addr.getSimpleValueType();

    //if (Addr.getOpcode() == RISCVISD::ADD_LO) {
    //    Base = Addr.getOperand(0);
    //    Offset = Addr.getOperand(1);
    //    return true;
    //}

    // Base + simm12 の形を認識
    if (CurDAG->isBaseWithConstantOffset(Addr)) {
        int64_t CVal = cast<ConstantSDNode>(Addr.getOperand(1))->getSExtValue();
        if (isInt<12>(CVal)) {
            Base = Addr.getOperand(0);

            // フレームインデックスのときはTargetFrameIndexに変換
            if (auto *FIN = dyn_cast<FrameIndexSDNode>(Base))
                Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), VT);

            /*if (Base.getOpcode() == RISCVISD::ADD_LO) {
                SDValue LoOperand = Base.getOperand(1);
                // フレームインデックスのときはTargetFrameIndexに変換
                if (auto *GA = dyn_cast<GlobalAddressSDNode>(LoOperand)) {
                    // If the Lo in (ADD_LO hi, lo) is a global variable's address
                    // (its low part, really), then we can rely on the alignment of that
                    // variable to provide a margin of safety before low part can overflow
                    // the 12 bits of the load/store offset. Check if CVal falls within
                    // that margin; if so (low part + CVal) can't overflow.
                    const DataLayout &DL = CurDAG->getDataLayout();
                    Align Alignment = commonAlignment(
                        GA->getGlobal()->getPointerAlignment(DL), GA->getOffset());
                    if (CVal == 0 || Alignment > CVal) {
                        int64_t CombinedOffset = CVal + GA->getOffset();
                        Base = Base.getOperand(0);
                        Offset = CurDAG->getTargetGlobalAddress(
                            GA->getGlobal(), SDLoc(LoOperand), LoOperand.getValueType(),
                            CombinedOffset, GA->getTargetFlags());
                        return true;
                    }
                }
            }

            if (auto *FIN = dyn_cast<FrameIndexSDNode>(Base))
                Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), VT);
            */
            Offset = CurDAG->getTargetConstant(CVal, DL, VT);
            return true;
        }

        // RV32Iではsimm12を超えるオフセットを直接使えないため無視
        return false;
    }

    /*
    // Handle ADD with large immediates.
    if (Addr.getOpcode() == ISD::ADD && isa<ConstantSDNode>(Addr.getOperand(1))) {
        int64_t CVal = cast<ConstantSDNode>(Addr.getOperand(1))->getSExtValue();
        assert(!isInt<12>(CVal) && "simm12 not already handled?");

        // Handle immediates in the range [-4096,-2049] or [2048, 4094]. We can use
        // an ADDI for part of the offset and fold the rest into the load/store.
        // This mirrors the AddiPair PatFrag in RISCVInstrInfo.td.
        if (isInt<12>(CVal / 2) && isInt<12>(CVal - CVal / 2)) {
            int64_t Adj = CVal < 0 ? -2048 : 2047;
            Base = SDValue(
                CurDAG->getMachineNode(RISCV::ADDI, DL, VT, Addr.getOperand(0),
                                        CurDAG->getTargetConstant(Adj, DL, VT)),
                                        0);
            Offset = CurDAG->getTargetConstant(CVal - Adj, DL, VT);
            return true;
        }

        // For larger immediates, we might be able to save one instruction from
        // constant materialization by folding the Lo12 bits of the immediate into
        // the address. We should only do this if the ADD is only used by loads and
        // stores that can fold the lo12 bits. Otherwise, the ADD will get iseled
        // separately with the full materialized immediate creating extra
        // instructions.
        if (isWorthFoldingAdd(Addr) &&
            selectConstantAddr(CurDAG, DL, VT, Subtarget, Addr.getOperand(1), Base,
                           Offset)) {
            // Insert an ADD instruction with the materialized Hi52 bits.
            Base = SDValue(
                CurDAG->getMachineNode(RISCV::ADD, DL, VT, Addr.getOperand(0), Base),
                0);
            return true;
        }
    }

    if (selectConstantAddr(CurDAG, DL, VT, Subtarget, Addr, Base, Offset))
        return true;
    */

    Base = Addr;
    Offset = CurDAG->getTargetConstant(0, DL, VT);
    return true;
}



// This pass converts a legalized DAG into a RISCV-specific DAG, ready
// for instruction scheduling.
FunctionPass *llvm::createMiniRvccISelDag(MiniRvccTargetMachine &TM,
                                       CodeGenOpt::Level OptLevel) {
    return new MiniRvccDAGToDAGISel(TM, OptLevel);
}
