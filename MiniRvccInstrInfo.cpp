//===-- MiniRvccInstrInfo.cpp - MiniRvcc Instruction Information ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the MiniRvcc implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "MiniRvccInstrInfo.h"
#include "MCTargetDesc/MiniRvccMatInt.h"
#include "MiniRvcc.h"
#include "MiniRvccMachineFunctionInfo.h"
#include "MiniRvccSubtarget.h"
#include "MiniRvccTargetMachine.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/CodeGen/LiveIntervals.h"
#include "llvm/CodeGen/LiveVariables.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GEN_CHECK_COMPRESS_INSTR
#include "MiniRvccGenCompressInstEmitter.inc"

#define GET_INSTRINFO_CTOR_DTOR
#define GET_INSTRINFO_NAMED_OPS
#include "MiniRvccGenInstrInfo.inc"

//static cl::opt<bool> PreferWholeRegisterMove(
//    "minirvcc-prefer-whole-register-move", cl::init(false), cl::Hidden,
//    cl::desc("Prefer whole register move for vector registers."));

//namespace llvm {
//namespace MiniRvccVPseudosTable {
//
//using namespace MiniRvcc;
//
//#define GET_MiniRvccVPseudosTable_IMPL
//#include "MiniRvccGenSearchableTables.inc"
//
//} // namespace MiniRvccVPseudosTable
//} // namespace llvm

MiniRvccInstrInfo::MiniRvccInstrInfo(MiniRvccSubtarget &STI)
   : MiniRvccGenInstrInfo(MiniRvcc::ADJCALLSTACKDOWN, MiniRvcc::ADJCALLSTACKUP),
      STI(STI) {}


///
/// 命令列中に挿入される NOP 命令を返す。
/// 標準の ADDI x0, x0, 0 を返す。
///
MCInst MiniRvccInstrInfo::getNop() const {
   //if (STI.getFeatureBits()[MiniRvcc::FeatureStdExtC])
   //   return MCInstBuilder(MiniRvcc::C_NOP);
   return MCInstBuilder(MiniRvcc::ADDI)
            .addReg(MiniRvcc::X0)
            .addReg(MiniRvcc::X0)
            .addImm(0);
}


///
/// 命令が対象とするレジスタが、スタック領域からロードされるかどうかを判定する。
/// スタックスロットからのロード命令であれば、そのスロットのインデックスを返す。
/// 該当しなければ 0 を返す。
///
/// @param MI         判定対象の命令
/// @param FrameIndex スタックスロットのインデックス（出力）
/// @return           スタックスロットからのロード対象となるレジスタ番号、
///                   ロード命令でなければ0を返す
///
/*
関数のスタックフレーム（メモリ領域）
   +-----------------------+ ← スタックポインタ (SP)
   |  引数                 |
   +-----------------------+
   |  戻りアドレス         |
   +-----------------------+
   |  保存レジスタ         |
   +-----------------------+ ← FrameIndex = 0 (例)
   |  ローカル変数1        |
   +-----------------------+ ← FrameIndex = 1 (例)
   |  ローカル変数2        |
   +-----------------------+ ← FrameIndex = 2 (例)
   |  ...                  |
   +-----------------------+
*/
///
unsigned MiniRvccInstrInfo::isLoadFromStackSlot(
   const MachineInstr &MI,
   int &FrameIndex) const 
{
   // ロード命令かチェックする
   switch (MI.getOpcode()) {
   default:
      return 0;
   case MiniRvcc::LB:
   case MiniRvcc::LBU:
   case MiniRvcc::LH:
   case MiniRvcc::LHU:
   //case MiniRvcc::FLH:
   case MiniRvcc::LW:
   //case MiniRvcc::FLW:
   //case MiniRvcc::LWU:
   //case MiniRvcc::LD:
   //case MiniRvcc::FLD:
      break;
   }

   // 第2オペランドがフレームインデックス（スタックスロット）を表し、
   // 第3オペランドが即値のオフセットで、その値が0の場合のみ処理を進める
   if (MI.getOperand(1).isFI() 
      && MI.getOperand(2).isImm() 
      && MI.getOperand(2).getImm() == 0) 
   {
      // フレームインデックス（スタックスロットの識別子）を設定
      FrameIndex = MI.getOperand(1).getIndex();
      // ロード命令の宛先レジスタを返却
      return MI.getOperand(0).getReg();
   }

   return 0;
}


///
/// 命令がスタックスロットへのストア命令か判定する。
/// スタックスロットとは、関数のスタックフレーム内の
/// ローカル変数や保存領域を指す仮想的なメモリ領域のこと。
///
/// @param MI         判定対象の命令
/// @param FrameIndex スタックスロットのインデックス（出力）
/// @return           スタックスロットからのストア対象となるレジスタ番号、
///                   ストア命令でなければ0を返す
///
unsigned MiniRvccInstrInfo::isStoreToStackSlot(
   const MachineInstr &MI,
   int &FrameIndex) const 
{

   // ストア命令かどうかチェック
   switch (MI.getOpcode()) {
   default:
      return 0;
   case MiniRvcc::SB:
   case MiniRvcc::SH:
   case MiniRvcc::SW:
   //case MiniRvcc::FSH:
   //case MiniRvcc::FSW:
   //case MiniRvcc::SD:
   //case MiniRvcc::FSD:
      break;
   }

   // 第2オペランドがFrameIndexを示すフレームインデックスかつ
   // 第3オペランドが即値で0（オフセットなし）である場合に限り処理を行う
   if (MI.getOperand(1).isFI() 
      && MI.getOperand(2).isImm() 
      && MI.getOperand(2).getImm() == 0) 
   {
      // スタックスロットのインデックスを取得
      FrameIndex = MI.getOperand(1).getIndex();
      // ストア元のレジスタ番号を返す
      return MI.getOperand(0).getReg();
   }

   return 0;
}


//static bool forwardCopyWillClobberTuple(
//   unsigned DstReg, unsigned SrcReg,
//   unsigned NumRegs) 
//{
//   return DstReg > SrcReg && (DstReg - SrcReg) < NumRegs;
//}


/*static bool isConvertibleToVMV_V_V(const MiniRvccSubtarget &STI,
                                   const MachineBasicBlock &MBB,
                                   MachineBasicBlock::const_iterator MBBI,
                                   MachineBasicBlock::const_iterator &DefMBBI,
                                   MiniRvccII::VLMUL LMul) {
  if (PreferWholeRegisterMove)
    return false;

  assert(MBBI->getOpcode() == TargetOpcode::COPY &&
         "Unexpected COPY instruction.");
  Register SrcReg = MBBI->getOperand(1).getReg();
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();

  bool FoundDef = false;
  bool FirstVSetVLI = false;
  unsigned FirstSEW = 0;
  while (MBBI != MBB.begin()) {
    --MBBI;
    if (MBBI->isMetaInstruction())
      continue;

    if (MBBI->getOpcode() == MiniRvcc::PseudoVSETVLI ||
        MBBI->getOpcode() == MiniRvcc::PseudoVSETVLIX0 ||
        MBBI->getOpcode() == MiniRvcc::PseudoVSETIVLI) {
      // There is a vsetvli between COPY and source define instruction.
      // vy = def_vop ...  (producing instruction)
      // ...
      // vsetvli
      // ...
      // vx = COPY vy
      if (!FoundDef) {
        if (!FirstVSetVLI) {
          FirstVSetVLI = true;
          unsigned FirstVType = MBBI->getOperand(2).getImm();
          MiniRvccII::VLMUL FirstLMul = MiniRvccVType::getVLMUL(FirstVType);
          FirstSEW = MiniRvccVType::getSEW(FirstVType);
          // The first encountered vsetvli must have the same lmul as the
          // register class of COPY.
          if (FirstLMul != LMul)
            return false;
        }
        // Only permit `vsetvli x0, x0, vtype` between COPY and the source
        // define instruction.
        if (MBBI->getOperand(0).getReg() != MiniRvcc::X0)
          return false;
        if (MBBI->getOperand(1).isImm())
          return false;
        if (MBBI->getOperand(1).getReg() != MiniRvcc::X0)
          return false;
        continue;
      }

      // MBBI is the first vsetvli before the producing instruction.
      unsigned VType = MBBI->getOperand(2).getImm();
      // If there is a vsetvli between COPY and the producing instruction.
      if (FirstVSetVLI) {
        // If SEW is different, return false.
        if (MiniRvccVType::getSEW(VType) != FirstSEW)
          return false;
      }

      // If the vsetvli is tail undisturbed, keep the whole register move.
      if (!MiniRvccVType::isTailAgnostic(VType))
        return false;

      // The checking is conservative. We only have register classes for
      // LMUL = 1/2/4/8. We should be able to convert vmv1r.v to vmv.v.v
      // for fractional LMUL operations. However, we could not use the vsetvli
      // lmul for widening operations. The result of widening operation is
      // 2 x LMUL.
      return LMul == MiniRvccVType::getVLMUL(VType);
    } else if (MBBI->isInlineAsm() || MBBI->isCall()) {
      return false;
    } else if (MBBI->getNumDefs()) {
      // Check all the instructions which will change VL.
      // For example, vleff has implicit def VL.
      if (MBBI->modifiesRegister(MiniRvcc::VL))
        return false;

      // Only converting whole register copies to vmv.v.v when the defining
      // value appears in the explicit operands.
      for (const MachineOperand &MO : MBBI->explicit_operands()) {
        if (!MO.isReg() || !MO.isDef())
          continue;
        if (!FoundDef && TRI->isSubRegisterEq(MO.getReg(), SrcReg)) {
          // We only permit the source of COPY has the same LMUL as the defined
          // operand.
          // There are cases we need to keep the whole register copy if the LMUL
          // is different.
          // For example,
          // $x0 = PseudoVSETIVLI 4, 73   // vsetivli zero, 4, e16,m2,ta,m
          // $v28m4 = PseudoVWADD_VV_M2 $v26m2, $v8m2
          // # The COPY may be created by vlmul_trunc intrinsic.
          // $v26m2 = COPY renamable $v28m2, implicit killed $v28m4
          //
          // After widening, the valid value will be 4 x e32 elements. If we
          // convert the COPY to vmv.v.v, it will only copy 4 x e16 elements.
          // FIXME: The COPY of subregister of Zvlsseg register will not be able
          // to convert to vmv.v.[v|i] under the constraint.
          if (MO.getReg() != SrcReg)
            return false;

          // In widening reduction instructions with LMUL_1 input vector case,
          // only checking the LMUL is insufficient due to reduction result is
          // always LMUL_1.
          // For example,
          // $x11 = PseudoVSETIVLI 1, 64 // vsetivli a1, 1, e8, m1, ta, mu
          // $v8m1 = PseudoVWREDSUM_VS_M1 $v26, $v27
          // $v26 = COPY killed renamable $v8
          // After widening, The valid value will be 1 x e16 elements. If we
          // convert the COPY to vmv.v.v, it will only copy 1 x e8 elements.
          uint64_t TSFlags = MBBI->getDesc().TSFlags;
          if (MiniRvccII::isRVVWideningReduction(TSFlags))
            return false;

          // Found the definition.
          FoundDef = true;
          DefMBBI = MBBI;
          // If the producing instruction does not depend on vsetvli, do not
          // convert COPY to vmv.v.v. For example, VL1R_V or PseudoVRELOAD.
          if (!MiniRvccII::hasSEWOp(TSFlags))
            return false;
          break;
        }
      }
    }
  }

  return false;
}*/

///
/// コピー命令を生成する。
/// DstReg に SrcReg の値をコピーする。
/// RV32Iでは、整数レジスタ間は ADDI 命令でコピーし、
/// CSRからGPRへのコピーもサポートする。
/// 
void MiniRvccInstrInfo::copyPhysReg(
   MachineBasicBlock &MBB,
   MachineBasicBlock::iterator MBBI,
   const DebugLoc &DL, 
   MCRegister DstReg,
   MCRegister SrcReg, 
   bool KillSrc) const 
{
   // 整数レジスタクラス間のコピーは ADDI x0, x0, 0 を使って実装
   if (MiniRvcc::GPRRegClass.contains(DstReg, SrcReg)) {
      BuildMI(MBB, MBBI, DL, get(MiniRvcc::ADDI), DstReg)
               .addReg(SrcReg, getKillRegState(KillSrc))
               .addImm(0);
      return;
   }

   // CSR（制御状態レジスタ）からGPRへのコピーを処理
   if (MiniRvcc::VCSRRegClass.contains(SrcReg) 
      && MiniRvcc::GPRRegClass.contains(DstReg)) 
   {
      const TargetRegisterInfo &TRI = *STI.getRegisterInfo();
      BuildMI(MBB, MBBI, DL, get(MiniRvcc::CSRRS), DstReg)
               .addImm(MiniRvccSysReg::lookupSysRegByName(TRI.getName(SrcReg))->Encoding)
               .addReg(MiniRvcc::X0);
      return;
   }

   /*
   // FPR->FPR copies and VR->VR copies.
   unsigned Opc;
   bool IsScalableVector = true;
   unsigned NF = 1;
   MiniRvccII::VLMUL LMul = MiniRvccII::LMUL_1;
   unsigned SubRegIdx = MiniRvcc::sub_vrm1_0;
   if (MiniRvcc::FPR16RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::FSGNJ_H;
      IsScalableVector = false;
   } else if (MiniRvcc::FPR32RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::FSGNJ_S;
      IsScalableVector = false;
   } else if (MiniRvcc::FPR64RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::FSGNJ_D;
      IsScalableVector = false;
   } else if (MiniRvcc::VRRegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV1R_V;
      LMul = MiniRvccII::LMUL_1;
   } else if (MiniRvcc::VRM2RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV2R_V;
      LMul = MiniRvccII::LMUL_2;
   } else if (MiniRvcc::VRM4RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV4R_V;
      LMul = MiniRvccII::LMUL_4;
   } else if (MiniRvcc::VRM8RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV8R_V;
      LMul = MiniRvccII::LMUL_8;
   } else if (MiniRvcc::VRN2M1RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV1R_V;
      SubRegIdx = MiniRvcc::sub_vrm1_0;
      NF = 2;
      LMul = MiniRvccII::LMUL_1;
   } else if (MiniRvcc::VRN2M2RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV2R_V;
      SubRegIdx = MiniRvcc::sub_vrm2_0;
      NF = 2;
      LMul = MiniRvccII::LMUL_2;
   } else if (MiniRvcc::VRN2M4RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV4R_V;
      SubRegIdx = MiniRvcc::sub_vrm4_0;
      NF = 2;
      LMul = MiniRvccII::LMUL_4;
   } else if (MiniRvcc::VRN3M1RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV1R_V;
      SubRegIdx = MiniRvcc::sub_vrm1_0;
      NF = 3;
      LMul = MiniRvccII::LMUL_1;
   } else if (MiniRvcc::VRN3M2RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV2R_V;
      SubRegIdx = MiniRvcc::sub_vrm2_0;
      NF = 3;
      LMul = MiniRvccII::LMUL_2;
   } else if (MiniRvcc::VRN4M1RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV1R_V;
      SubRegIdx = MiniRvcc::sub_vrm1_0;
      NF = 4;
      LMul = MiniRvccII::LMUL_1;
   } else if (MiniRvcc::VRN4M2RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV2R_V;
      SubRegIdx = MiniRvcc::sub_vrm2_0;
      NF = 4;
      LMul = MiniRvccII::LMUL_2;
   } else if (MiniRvcc::VRN5M1RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV1R_V;
      SubRegIdx = MiniRvcc::sub_vrm1_0;
      NF = 5;
      LMul = MiniRvccII::LMUL_1;
   } else if (MiniRvcc::VRN6M1RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV1R_V;
      SubRegIdx = MiniRvcc::sub_vrm1_0;
      NF = 6;
      LMul = MiniRvccII::LMUL_1;
   } else if (MiniRvcc::VRN7M1RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV1R_V;
      SubRegIdx = MiniRvcc::sub_vrm1_0;
      NF = 7;
      LMul = MiniRvccII::LMUL_1;
   } else if (MiniRvcc::VRN8M1RegClass.contains(DstReg, SrcReg)) {
      Opc = MiniRvcc::PseudoVMV1R_V;
      SubRegIdx = MiniRvcc::sub_vrm1_0;
      NF = 8;
      LMul = MiniRvccII::LMUL_1;
   } else {
      llvm_unreachable("Impossible reg-to-reg copy");
   }
   */

   /*
   if (IsScalableVector) {
      bool UseVMV_V_V = false;
      MachineBasicBlock::const_iterator DefMBBI;
      unsigned DefExplicitOpNum;
      unsigned VIOpc;
      if (isConvertibleToVMV_V_V(STI, MBB, MBBI, DefMBBI, LMul)) {
         UseVMV_V_V = true;
         DefExplicitOpNum = DefMBBI->getNumExplicitOperands();
         // We only need to handle LMUL = 1/2/4/8 here because we only define
         // vector register classes for LMUL = 1/2/4/8.
         switch (LMul) {
         default:
            llvm_unreachable("Impossible LMUL for vector register copy.");
         case MiniRvccII::LMUL_1:
            Opc = MiniRvcc::PseudoVMV_V_V_M1;
            VIOpc = MiniRvcc::PseudoVMV_V_I_M1;
            break;
         case MiniRvccII::LMUL_2:
            Opc = MiniRvcc::PseudoVMV_V_V_M2;
            VIOpc = MiniRvcc::PseudoVMV_V_I_M2;
            break;
         case MiniRvccII::LMUL_4:
            Opc = MiniRvcc::PseudoVMV_V_V_M4;
            VIOpc = MiniRvcc::PseudoVMV_V_I_M4;
            break;
         case MiniRvccII::LMUL_8:
            Opc = MiniRvcc::PseudoVMV_V_V_M8;
            VIOpc = MiniRvcc::PseudoVMV_V_I_M8;
            break;
         }
      }

      bool UseVMV_V_I = false;
      if (UseVMV_V_V && (DefMBBI->getOpcode() == VIOpc)) {
         UseVMV_V_I = true;
         Opc = VIOpc;
      }

      if (NF == 1) {
         auto MIB = BuildMI(MBB, MBBI, DL, get(Opc), DstReg);
         if (UseVMV_V_I)
            MIB = MIB.add(DefMBBI->getOperand(1));
         else
            MIB = MIB.addReg(SrcReg, getKillRegState(KillSrc));
         if (UseVMV_V_V) {
            // The last two arguments of vector instructions are
            // AVL, SEW. We also need to append the implicit-use vl and vtype.
            MIB.add(DefMBBI->getOperand(DefExplicitOpNum - 2)); // AVL
            MIB.add(DefMBBI->getOperand(DefExplicitOpNum - 1)); // SEW
            MIB.addReg(MiniRvcc::VL, RegState::Implicit);
            MIB.addReg(MiniRvcc::VTYPE, RegState::Implicit);
         }
      } else {
         const TargetRegisterInfo *TRI = STI.getRegisterInfo();

         int I = 0, End = NF, Incr = 1;
         unsigned SrcEncoding = TRI->getEncodingValue(SrcReg);
         unsigned DstEncoding = TRI->getEncodingValue(DstReg);
         unsigned LMulVal;
         bool Fractional;
         std::tie(LMulVal, Fractional) = MiniRvccVType::decodeVLMUL(LMul);
         assert(!Fractional && "It is impossible be fractional lmul here.");
         if (forwardCopyWillClobberTuple(DstEncoding, SrcEncoding, NF * LMulVal)) {
            I = NF - 1;
            End = -1;
            Incr = -1;
         }

         for (; I != End; I += Incr) {
            auto MIB = BuildMI(MBB, MBBI, DL, get(Opc),
                           TRI->getSubReg(DstReg, SubRegIdx + I));
            if (UseVMV_V_I)
               MIB = MIB.add(DefMBBI->getOperand(1));
            else
               MIB = MIB.addReg(TRI->getSubReg(SrcReg, SubRegIdx + I),
                           getKillRegState(KillSrc));
            if (UseVMV_V_V) {
               MIB.add(DefMBBI->getOperand(DefExplicitOpNum - 2)); // AVL
               MIB.add(DefMBBI->getOperand(DefExplicitOpNum - 1)); // SEW
               MIB.addReg(MiniRvcc::VL, RegState::Implicit);
               MIB.addReg(MiniRvcc::VTYPE, RegState::Implicit);
            }
         }
      }
   } else {
      BuildMI(MBB, MBBI, DL, get(Opc), DstReg)
         .addReg(SrcReg, getKillRegState(KillSrc))
         .addReg(SrcReg, getKillRegState(KillSrc));
   }
   */

   llvm_unreachable("Impossible reg-to-reg copy");
}

///
/// Store the specified register of the given register class to the specified
/// stack frame index. The store instruction is to be added to the given
/// machine basic block before the specified machine instruction. If isKill
/// is true, the register operand is the last use and must be marked kill.
///
void MiniRvccInstrInfo::storeRegToStackSlot(
  MachineBasicBlock &MBB,
  MachineBasicBlock::iterator I,
  Register SrcReg, 
  bool IsKill, int FI,
  const TargetRegisterClass *RC,
  const TargetRegisterInfo *TRI) const 
{
  // デバッグ情報の初期化
  // 挿入位置 I の命令が有効であれば、
  // その命令からデバッグ位置情報を取得する
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  // この基本ブロック（MBB）を含む親の 
  // MachineFunction（関数単位の中間表現）を取得。
  MachineFunction *MF = MBB.getParent();

  // MachineFunction に紐づくスタックフレーム情報
  //（ローカル変数や引数の保存先など）を取得。
  MachineFrameInfo &MFI = MF->getFrameInfo();

  unsigned Opcode;

  //bool IsScalableVector = true;
  //bool IsZvlsseg = true;

  if (MiniRvcc::GPRRegClass.hasSubClassEq(RC)) {
    //Opcode = TRI->getRegSizeInBits(MiniRvcc::GPRRegClass) == 32 ?
    //         MiniRvcc::SW : MiniRvcc::SD;
    //IsScalableVector = false;

    // 指定されたRCが汎用レジスタ(GPR)に属しているか確認.
    // RV32Iの場合, GPRは32bitなので、ストア命令にSWを選択.
    Opcode = MiniRvcc::SW;
  } /*else if (MiniRvcc::FPR16RegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::FSH;
    IsScalableVector = false;
  } else if (MiniRvcc::FPR32RegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::FSW;
    IsScalableVector = false;
  } else if (MiniRvcc::FPR64RegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::FSD;
    IsScalableVector = false;
  } else if (MiniRvcc::VRRegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::PseudoVSPILL_M1;
    IsZvlsseg = false;
  } else if (MiniRvcc::VRM2RegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::PseudoVSPILL_M2;
    IsZvlsseg = false;
  } else if (MiniRvcc::VRM4RegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::PseudoVSPILL_M4;
    IsZvlsseg = false;
  } else if (MiniRvcc::VRM8RegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::PseudoVSPILL_M8;
    IsZvlsseg = false;
  } else if (MiniRvcc::VRN2M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVSPILL2_M1;
  else if (MiniRvcc::VRN2M2RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVSPILL2_M2;
  else if (MiniRvcc::VRN2M4RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVSPILL2_M4;
  else if (MiniRvcc::VRN3M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVSPILL3_M1;
  else if (MiniRvcc::VRN3M2RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVSPILL3_M2;
  else if (MiniRvcc::VRN4M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVSPILL4_M1;
  else if (MiniRvcc::VRN4M2RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVSPILL4_M2;
  else if (MiniRvcc::VRN5M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVSPILL5_M1;
  else if (MiniRvcc::VRN6M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVSPILL6_M1;
  else if (MiniRvcc::VRN7M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVSPILL7_M1;
  else if (MiniRvcc::VRN8M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVSPILL8_M1;*/
  else
    llvm_unreachable("Can't store this register to stack slot");

  /*if (IsScalableVector) {
    MachineMemOperand *MMO = MF->getMachineMemOperand(
        MachinePointerInfo::getFixedStack(*MF, FI), MachineMemOperand::MOStore,
        MemoryLocation::UnknownSize, MFI.getObjectAlign(FI));

    MFI.setStackID(FI, TargetStackID::ScalableVector);
    auto MIB = BuildMI(MBB, I, DL, get(Opcode))
                   .addReg(SrcReg, getKillRegState(IsKill))
                   .addFrameIndex(FI)
                   .addMemOperand(MMO);
    if (IsZvlsseg) {
      // For spilling/reloading Zvlsseg registers, append the dummy field for
      // the scaled vector length. The argument will be used when expanding
      // these pseudo instructions.
      MIB.addReg(MiniRvcc::X0);
    }
  } else {*/

    // 命令がアクセスするメモリの詳細情報取得
    MachineMemOperand *MMO = MF->getMachineMemOperand(
        MachinePointerInfo::getFixedStack(*MF, FI), // メモリアドレス情報：固定スタックのFI番目の場所
        MachineMemOperand::MOStore,   // メモリ操作タイプ：ストア（書き込み）
        MFI.getObjectSize(FI),        // メモリサイズ：スタックオブジェクトのサイズ
        MFI.getObjectAlign(FI));      // メモリアライメント：スタックオブジェクトのアライメント

    // ストア命令の挿入
    BuildMI(MBB, I, DL, get(Opcode))
        .addReg(SrcReg, getKillRegState(IsKill))
        .addFrameIndex(FI)
        .addImm(0)
        .addMemOperand(MMO);
  /*}*/
}


///
/// Load the specified register of the given register class from the specified
/// stack frame index. The load instruction is to be added to the given
/// machine basic block before the specified machine instruction.
///
void MiniRvccInstrInfo::loadRegFromStackSlot(
  MachineBasicBlock &MBB,
  MachineBasicBlock::iterator I,
  Register DstReg, int FI,
  const TargetRegisterClass *RC,
  const TargetRegisterInfo *TRI) const 
{
  // デバッグ情報の初期化
  // 挿入位置 I の命令が有効であれば、
  // その命令からデバッグ位置情報を取得する
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  // この基本ブロック（MBB）を含む親の 
  // MachineFunction（関数単位の中間表現）を取得。
  MachineFunction *MF = MBB.getParent();

  // MachineFunction に紐づくスタックフレーム情報
  //（ローカル変数や引数の保存先など）を取得。
  MachineFrameInfo &MFI = MF->getFrameInfo();

  unsigned Opcode;
  //bool IsScalableVector = true;
  //bool IsZvlsseg = true;

  if (MiniRvcc::GPRRegClass.hasSubClassEq(RC)) {
    //Opcode = TRI->getRegSizeInBits(MiniRvcc::GPRRegClass) == 32 ?
    //         MiniRvcc::LW : MiniRvcc::LD;
    //IsScalableVector = false;

    // 指定されたRCが汎用レジスタ(GPR)に属しているか確認.
    // RV32Iの場合, GPRは32bitなので、ロード命令にLWを選択.
    Opcode = MiniRvcc::LW;
  } /*else if (MiniRvcc::FPR16RegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::FLH;
    IsScalableVector = false;
  } else if (MiniRvcc::FPR32RegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::FLW;
    IsScalableVector = false;
  } else if (MiniRvcc::FPR64RegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::FLD;
    IsScalableVector = false;
  } else if (MiniRvcc::VRRegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::PseudoVRELOAD_M1;
    IsZvlsseg = false;
  } else if (MiniRvcc::VRM2RegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::PseudoVRELOAD_M2;
    IsZvlsseg = false;
  } else if (MiniRvcc::VRM4RegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::PseudoVRELOAD_M4;
    IsZvlsseg = false;
  } else if (MiniRvcc::VRM8RegClass.hasSubClassEq(RC)) {
    Opcode = MiniRvcc::PseudoVRELOAD_M8;
    IsZvlsseg = false;
  } else if (MiniRvcc::VRN2M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVRELOAD2_M1;
  else if (MiniRvcc::VRN2M2RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVRELOAD2_M2;
  else if (MiniRvcc::VRN2M4RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVRELOAD2_M4;
  else if (MiniRvcc::VRN3M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVRELOAD3_M1;
  else if (MiniRvcc::VRN3M2RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVRELOAD3_M2;
  else if (MiniRvcc::VRN4M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVRELOAD4_M1;
  else if (MiniRvcc::VRN4M2RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVRELOAD4_M2;
  else if (MiniRvcc::VRN5M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVRELOAD5_M1;
  else if (MiniRvcc::VRN6M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVRELOAD6_M1;
  else if (MiniRvcc::VRN7M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVRELOAD7_M1;
  else if (MiniRvcc::VRN8M1RegClass.hasSubClassEq(RC))
    Opcode = MiniRvcc::PseudoVRELOAD8_M1;*/
  else
    llvm_unreachable("Can't load this register from stack slot");

  /*if (IsScalableVector) {
    MachineMemOperand *MMO = MF->getMachineMemOperand(
        MachinePointerInfo::getFixedStack(*MF, FI), MachineMemOperand::MOLoad,
        MemoryLocation::UnknownSize, MFI.getObjectAlign(FI));

    MFI.setStackID(FI, TargetStackID::ScalableVector);
    auto MIB = BuildMI(MBB, I, DL, get(Opcode), DstReg)
                   .addFrameIndex(FI)
                   .addMemOperand(MMO);
    if (IsZvlsseg) {
      // For spilling/reloading Zvlsseg registers, append the dummy field for
      // the scaled vector length. The argument will be used when expanding
      // these pseudo instructions.
      MIB.addReg(MiniRvcc::X0);
    }
  } else {*/

    // 命令がアクセスするメモリの詳細情報取得
    MachineMemOperand *MMO = MF->getMachineMemOperand(
        MachinePointerInfo::getFixedStack(*MF, FI), MachineMemOperand::MOLoad,
        MFI.getObjectSize(FI), MFI.getObjectAlign(FI));

    // ロード命令の挿入
    BuildMI(MBB, I, DL, get(Opcode), DstReg)
        .addFrameIndex(FI)
        .addImm(0)
        .addMemOperand(MMO);
  /*}*/
}

/*MachineInstr *MiniRvccInstrInfo::foldMemoryOperandImpl(
    MachineFunction &MF, MachineInstr &MI, ArrayRef<unsigned> Ops,
    MachineBasicBlock::iterator InsertPt, int FrameIndex, LiveIntervals *LIS,
    VirtRegMap *VRM) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  // The below optimizations narrow the load so they are only valid for little
  // endian.
  // TODO: Support big endian by adding an offset into the frame object?
  if (MF.getDataLayout().isBigEndian())
    return nullptr;

  // Fold load from stack followed by sext.w into lw.
  // TODO: Fold with sext.b, sext.h, zext.b, zext.h, zext.w?
  if (Ops.size() != 1 || Ops[0] != 1)
   return nullptr;

  unsigned LoadOpc;
  switch (MI.getOpcode()) {
  default:
    if (MiniRvcc::isSEXT_W(MI)) {
      LoadOpc = MiniRvcc::LW;
      break;
    }
    if (MiniRvcc::isZEXT_W(MI)) {
      LoadOpc = MiniRvcc::LWU;
      break;
    }
    if (MiniRvcc::isZEXT_B(MI)) {
      LoadOpc = MiniRvcc::LBU;
      break;
    }
    return nullptr;
  case MiniRvcc::SEXT_H:
    LoadOpc = MiniRvcc::LH;
    break;
  case MiniRvcc::SEXT_B:
    LoadOpc = MiniRvcc::LB;
    break;
  case MiniRvcc::ZEXT_H_RV32:
  case MiniRvcc::ZEXT_H_RV64:
    LoadOpc = MiniRvcc::LHU;
    break;
  }

  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  Register DstReg = MI.getOperand(0).getReg();
  return BuildMI(*MI.getParent(), InsertPt, MI.getDebugLoc(), get(LoadOpc),
                 DstReg)
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO);
}*/

void MiniRvccInstrInfo::movImm(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI,
                            const DebugLoc &DL, Register DstReg, uint64_t Val,
                            MachineInstr::MIFlag Flag) const {
  Register SrcReg = MiniRvcc::X0;

  if (!STI.is64Bit() && !isInt<32>(Val))
    report_fatal_error("Should only materialize 32-bit constants for RV32");

  MiniRvccMatInt::InstSeq Seq =
      MiniRvccMatInt::generateInstSeq(Val, STI.getFeatureBits());
  assert(!Seq.empty());

  for (MiniRvccMatInt::Inst &Inst : Seq) {
    switch (Inst.getOpndKind()) {
    case MiniRvccMatInt::Imm:
      BuildMI(MBB, MBBI, DL, get(Inst.Opc), DstReg)
          .addImm(Inst.Imm)
          .setMIFlag(Flag);
      break;
    case MiniRvccMatInt::RegX0:
      BuildMI(MBB, MBBI, DL, get(Inst.Opc), DstReg)
          .addReg(SrcReg, RegState::Kill)
          .addReg(MiniRvcc::X0)
          .setMIFlag(Flag);
      break;
    case MiniRvccMatInt::RegReg:
      BuildMI(MBB, MBBI, DL, get(Inst.Opc), DstReg)
          .addReg(SrcReg, RegState::Kill)
          .addReg(SrcReg, RegState::Kill)
          .setMIFlag(Flag);
      break;
    case MiniRvccMatInt::RegImm:
      BuildMI(MBB, MBBI, DL, get(Inst.Opc), DstReg)
          .addReg(SrcReg, RegState::Kill)
          .addImm(Inst.Imm)
          .setMIFlag(Flag);
      break;
    }

    // Only the first instruction has X0 as its source.
    SrcReg = DstReg;
  }
}

static MiniRvccCC::CondCode getCondFromBranchOpc(unsigned Opc) {
  switch (Opc) {
  default:
    return MiniRvccCC::COND_INVALID;
  case MiniRvcc::BEQ:
    return MiniRvccCC::COND_EQ;
  case MiniRvcc::BNE:
    return MiniRvccCC::COND_NE;
  case MiniRvcc::BLT:
    return MiniRvccCC::COND_LT;
  case MiniRvcc::BGE:
    return MiniRvccCC::COND_GE;
  case MiniRvcc::BLTU:
    return MiniRvccCC::COND_LTU;
  case MiniRvcc::BGEU:
    return MiniRvccCC::COND_GEU;
  }
}

// The contents of values added to Cond are not examined outside of
// MiniRvccInstrInfo, giving us flexibility in what to push to it. For MiniRvcc, we
// push BranchOpcode, Reg1, Reg2.
static void parseCondBranch(MachineInstr &LastInst, MachineBasicBlock *&Target,
                            SmallVectorImpl<MachineOperand> &Cond) {
  // Block ends with fall-through condbranch.
  assert(LastInst.getDesc().isConditionalBranch() &&
         "Unknown conditional branch");
  Target = LastInst.getOperand(2).getMBB();
  unsigned CC = getCondFromBranchOpc(LastInst.getOpcode());
  Cond.push_back(MachineOperand::CreateImm(CC));
  Cond.push_back(LastInst.getOperand(0));
  Cond.push_back(LastInst.getOperand(1));
}

const MCInstrDesc &MiniRvccInstrInfo::getBrCond(MiniRvccCC::CondCode CC) const {
  switch (CC) {
  default:
    llvm_unreachable("Unknown condition code!");
  case MiniRvccCC::COND_EQ:
    return get(MiniRvcc::BEQ);
  case MiniRvccCC::COND_NE:
    return get(MiniRvcc::BNE);
  case MiniRvccCC::COND_LT:
    return get(MiniRvcc::BLT);
  case MiniRvccCC::COND_GE:
    return get(MiniRvcc::BGE);
  case MiniRvccCC::COND_LTU:
    return get(MiniRvcc::BLTU);
  case MiniRvccCC::COND_GEU:
    return get(MiniRvcc::BGEU);
  }
}

MiniRvccCC::CondCode MiniRvccCC::getOppositeBranchCondition(MiniRvccCC::CondCode CC) {
  switch (CC) {
  default:
    llvm_unreachable("Unrecognized conditional branch");
  case MiniRvccCC::COND_EQ:
    return MiniRvccCC::COND_NE;
  case MiniRvccCC::COND_NE:
    return MiniRvccCC::COND_EQ;
  case MiniRvccCC::COND_LT:
    return MiniRvccCC::COND_GE;
  case MiniRvccCC::COND_GE:
    return MiniRvccCC::COND_LT;
  case MiniRvccCC::COND_LTU:
    return MiniRvccCC::COND_GEU;
  case MiniRvccCC::COND_GEU:
    return MiniRvccCC::COND_LTU;
  }
}


///
/// Analyze the branching code at the end of MBB, returning
/// true if it cannot be understood (e.g. it's a switch dispatch or isn't
/// implemented for a target).  Upon success, this returns false and returns
/// with the following information in various cases:
///
/// 1. If this block ends with no branches (it just falls through to its succ)
///    just return false, leaving TBB/FBB null.
/// 2. If this block ends with only an unconditional branch, it sets TBB to be
///    the destination block.
/// 3. If this block ends with a conditional branch and it falls through to a
///    successor block, it sets TBB to be the branch destination block and a
///    list of operands that evaluate the condition. These operands can be
///    passed to other TargetInstrInfo methods to create new branches.
/// 4. If this block ends with a conditional branch followed by an
///    unconditional branch, it returns the 'true' destination in TBB, the
///    'false' destination in FBB, and a list of operands that evaluate the
///    condition.  These operands can be passed to other TargetInstrInfo
///    methods to create new branches.
///
/// Note that removeBranch and insertBranch must be implemented to support
/// cases where this method returns success.
///
/// If AllowModify is true, then this routine is allowed to modify the basic
/// block (e.g. delete instructions after the unconditional branch).
///
/// The CFG information in MBB.Predecessors and MBB.Successors must be valid
/// before calling this function.
/// 
bool MiniRvccInstrInfo::analyzeBranch(
  MachineBasicBlock &MBB,
  MachineBasicBlock *&TBB,
  MachineBasicBlock *&FBB,
  SmallVectorImpl<MachineOperand> &Cond,
  bool AllowModify) const 
{
  TBB = FBB = nullptr;
  Cond.clear();

  // 最後の非デバッグ命令を取得
  // If the block has no terminators, it just falls into the block after it.
  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();

  // ブロック末尾が分岐命令でなければ、フォールスルー（fallthrough）扱い
  if (I == MBB.end() || !isUnpredicatedTerminator(*I))
    return false;


  // Terminator（分岐命令）数をカウントしつつ、
  // 最初の無条件 or 間接分岐を探す
  // Count the number of terminators and find the first unconditional or
  // indirect branch.
  MachineBasicBlock::iterator FirstUncondOrIndirectBr = MBB.end();
  int NumTerminators = 0;
  for (auto J = I.getReverse(); J != MBB.rend() && isUnpredicatedTerminator(*J); J++) {
    NumTerminators++;
    //--- 以下のようなelse分岐を探す
    //if (cond)
    // goto A;
    //else
    // goto B;
    if (J->getDesc().isUnconditionalBranch() || J->getDesc().isIndirectBranch()) {
      FirstUncondOrIndirectBr = J.getReverse();
    }
  }

  // AllowModifyが真の時の末尾命令削除は通常不要のためコメントアウト
  // If AllowModify is true, we can erase any terminators after
  // FirstUncondOrIndirectBR.
  /*if (AllowModify && FirstUncondOrIndirectBr != MBB.end()) {
    while (std::next(FirstUncondOrIndirectBr) != MBB.end()) {
      std::next(FirstUncondOrIndirectBr)->eraseFromParent();
      NumTerminators--;
    }
    I = FirstUncondOrIndirectBr;
  }*/

  // 間接分岐はreturn相当なので解析不能としてtrueを返す
  // We can't handle blocks that end in an indirect branch.
  //-------------------------------------
  //void foo(void (*func_ptr)()) {
  //    // 関数ポインタを使って間接的にジャンプ（関数呼び出し）
  //    func_ptr();
  //}
  //-------------------------------------
  if (I->getDesc().isIndirectBranch())
    return true;

  // RV32Iの設計上、分岐命令は最大2つまでを想定
  // We can't handle blocks with more than 2 terminators.
  if (NumTerminators > 2)
    return true;

  // 単一の無条件分岐（例：goto）
  //-------------------------------------
  //void foo() {
  //    goto LABEL;  // 無条件ジャンプ
  //
  //LABEL:
  //    // ラベル先の処理
  //    return;
  //}
  //-------------------------------------
  // Handle a single unconditional branch.
  if (NumTerminators == 1 && I->getDesc().isUnconditionalBranch()) {
    TBB = getBranchDestBlock(*I);
    return false;
  }

  // 単一の条件分岐（例：if）
  // Handle a single conditional branch.
  //-------------------------------------
  //void foo(int x) {
  //    if (x > 0) {
  //        goto POSITIVE;  // 条件付きジャンプ
  //    }
  //    // 条件が偽のときの処理
  //    return;
  //
  //POSITIVE:
  //    // 条件が真のときの処理
  //    return;
  //}
  //-------------------------------------
  if (NumTerminators == 1 && I->getDesc().isConditionalBranch()) {
    parseCondBranch(*I, TBB, Cond);
    return false;
  }

  // 条件分岐の後に無条件分岐（if-else構造）
  // Handle a conditional branch followed by an unconditional branch.
  //-------------------------------------
  //beq → 条件分岐（TrueDest = A）
  //jal → 無条件分岐（FalseDest = B）
  //-------------------------------------
  if (NumTerminators == 2 && std::prev(I)->getDesc().isConditionalBranch() &&
      I->getDesc().isUnconditionalBranch()) {
    parseCondBranch(*std::prev(I), TBB, Cond);
    FBB = getBranchDestBlock(*I);
    return false;
  }

  // それ以外の複雑な分岐は未対応（trueを返してエラー扱い）
  // Otherwise, we can't handle this.
  return true;
}


///
/// Remove the branching code at the end of the specific MBB.
/// This is only invoked in cases where analyzeBranch returns success. It
/// returns the number of instructions that were removed.
/// If \p BytesRemoved is non-null, report the change in code size from the
/// removed instructions.
/// 
/// 以下のようなブロック順の時に、本関数が呼び出される
/// 戻り値の数だけ分岐命令があり、それを削除対象とする
///--------------------------------------
//bb1:
//  ...
//  br bb2  ← 条件なしジャンプ
//
//bb2:
//  ...
///--------------------------------------
unsigned MiniRvccInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                         int *BytesRemoved) const 
{
  if (BytesRemoved)
    *BytesRemoved = 0;

  // ブロックの最後の非デバッグ命令を取得
  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
  if (I == MBB.end())
    return 0;

  // 最後の命令が分岐でなければ削除するものはない
  if (!I->getDesc().isUnconditionalBranch() &&
      !I->getDesc().isConditionalBranch())
    return 0;

  // 最後の分岐命令（無条件または条件）を削除
  // Remove the branch.
  if (BytesRemoved)
    *BytesRemoved += getInstSizeInBytes(*I);
  I->eraseFromParent();

  // 一つ前に戻ってさらに条件分岐があれば、それも削除
  I = MBB.end();
  if (I == MBB.begin())
    return 1;

  --I;
  if (!I->getDesc().isConditionalBranch())
    return 1;

  // Remove the branch.
  if (BytesRemoved)
    *BytesRemoved += getInstSizeInBytes(*I);
  I->eraseFromParent();
  return 2;
}



// Inserts a branch into the end of the specific MachineBasicBlock, returning
// the number of instructions inserted.
unsigned MiniRvccInstrInfo::insertBranch(
    MachineBasicBlock &MBB, MachineBasicBlock *TBB, MachineBasicBlock *FBB,
    ArrayRef<MachineOperand> Cond, const DebugLoc &DL, int *BytesAdded) const {
  if (BytesAdded)
    *BytesAdded = 0;

  // Shouldn't be a fall through.
  assert(TBB && "insertBranch must not be told to insert a fallthrough");
  assert((Cond.size() == 3 || Cond.size() == 0) &&
         "MiniRvcc branch conditions have two components!");

  // Unconditional branch.
  if (Cond.empty()) {
    MachineInstr &MI = *BuildMI(&MBB, DL, get(MiniRvcc::PseudoBR)).addMBB(TBB);
    if (BytesAdded)
      *BytesAdded += getInstSizeInBytes(MI);
    return 1;
  }

  // Either a one or two-way conditional branch.
  auto CC = static_cast<MiniRvccCC::CondCode>(Cond[0].getImm());
  MachineInstr &CondMI =
      *BuildMI(&MBB, DL, getBrCond(CC)).add(Cond[1]).add(Cond[2]).addMBB(TBB);
  if (BytesAdded)
    *BytesAdded += getInstSizeInBytes(CondMI);

  // One-way conditional branch.
  if (!FBB)
    return 1;

  // Two-way conditional branch.
  MachineInstr &MI = *BuildMI(&MBB, DL, get(MiniRvcc::PseudoBR)).addMBB(FBB);
  if (BytesAdded)
    *BytesAdded += getInstSizeInBytes(MI);
  return 2;
}


///
/// Insert an unconditional indirect branch at the end of \p MBB to \p
/// NewDestBB. Optionally, insert the clobbered register restoring in \p
/// RestoreBB. \p BrOffset indicates the offset of \p NewDestBB relative to
/// the offset of the position to insert the new branch.
/// 
/// LLVM がコード生成フェーズで「通常のジャンプが届かない場所へ分岐する」
//  必要があるときに使う、間接ジャンプ命令列の挿入です。
/// 
void MiniRvccInstrInfo::insertIndirectBranch(MachineBasicBlock &MBB,
                                             MachineBasicBlock &DestBB,
                                             MachineBasicBlock &RestoreBB,
                                             const DebugLoc &DL, int64_t BrOffset,
                                             RegScavenger *RS) const 
{

  /*
  //assert(RS && "RegScavenger required for long branching");
  assert(MBB.empty() && "new block should be inserted for expanding unconditional branch");
  assert(MBB.pred_size() == 1);


  // MachineFunctionとレジスタ管理情報を取得
  MachineFunction *MF = MBB.getParent();
  MachineRegisterInfo &MRI = MF->getRegInfo();

  if (!isInt<32>(BrOffset))
    report_fatal_error(
        "Branch offsets outside of the signed 32-bit range not supported");

  // FIXME: A virtual register must be used initially, as the register
  // scavenger won't work with empty blocks (SIInstrInfo::insertIndirectBranch
  // uses the same workaround).
  Register ScratchReg = MRI.createVirtualRegister(&MiniRvcc::GPRRegClass);
  auto II = MBB.end();

  MachineInstr &MI = *BuildMI(MBB, II, DL, get(MiniRvcc::PseudoJump))
                          .addReg(ScratchReg, RegState::Define | RegState::Dead)
                          .addMBB(&DestBB, MiniRvccII::MO_CALL);

  RS->enterBasicBlockEnd(MBB);
  Register Scav = RS->scavengeRegisterBackwards(MiniRvcc::GPRRegClass,
                                                MI.getIterator(), false, 0);
  // TODO: The case when there is no scavenged register needs special handling.
  assert(Scav != MiniRvcc::NoRegister && "No register is scavenged!");
  MRI.replaceRegWith(ScratchReg, Scav);
  MRI.clearVirtRegs();
  RS->setRegUsed(Scav);
  */

  // lui ScratchReg, %hi(DestBB)
  BuildMI(MBB, MBB.end(), DL, get(MiniRvcc::LUI), ScratchReg)
      .addGlobalAddress(DestBB.getSymbol(), 0, RISCVII::MO_HI);

  // addi ScratchReg, ScratchReg, %lo(DestBB)
  BuildMI(MBB, MBB.end(), DL, get(MiniRvcc::ADDI), ScratchReg)
      .addReg(ScratchReg)
      .addGlobalAddress(DestBB.getSymbol(), 0, RISCVII::MO_LO);

  // jalr x0, 0(ScratchReg)
  BuildMI(MBB, MBB.end(), DL, get(MiniRvcc::JALR))
      .addReg(RISCV::X0)       // 戻り先は使わない（破棄）
      .addReg(ScratchReg)      // ジャンプ先
      .addImm(0);              // オフセット 0

  return MBB.end();
}


///
/// Reverses the branch condition of the specified condition list,
/// returning false on success and true if it cannot be reversed.
///
/// 条件分岐の条件を反転するための関数
/// RV32Iのみ対応にしたい場合、使える分岐は BEQ/BNE/BLT/BGE/BLTU/BGEU の6種類に限定
/// 上記6種類だけ扱っていれば、今のコードでそのまま対応可能。
///
bool MiniRvccInstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const 
{
  assert((Cond.size() == 3) && "Invalid branch condition!");
  auto CC = static_cast<MiniRvccCC::CondCode>(Cond[0].getImm());
  Cond[0].setImm(getOppositeBranchCondition(CC));
  return false;
}


MachineBasicBlock *
MiniRvccInstrInfo::getBranchDestBlock(const MachineInstr &MI) const {
  assert(MI.getDesc().isBranch() && "Unexpected opcode!");
  // The branch target is always the last operand.
  int NumOp = MI.getNumExplicitOperands();
  return MI.getOperand(NumOp - 1).getMBB();
}

bool MiniRvccInstrInfo::isBranchOffsetInRange(unsigned BranchOp,
                                           int64_t BrOffset) const {
  unsigned XLen = STI.getXLen();
  // Ideally we could determine the supported branch offset from the
  // MiniRvccII::FormMask, but this can't be used for Pseudo instructions like
  // PseudoBR.
  switch (BranchOp) {
  default:
    llvm_unreachable("Unexpected opcode!");
  case MiniRvcc::BEQ:
  case MiniRvcc::BNE:
  case MiniRvcc::BLT:
  case MiniRvcc::BGE:
  case MiniRvcc::BLTU:
  case MiniRvcc::BGEU:
    return isIntN(13, BrOffset);
  case MiniRvcc::JAL:
  case MiniRvcc::PseudoBR:
    return isIntN(21, BrOffset);
  case MiniRvcc::PseudoJump:
    return isIntN(32, SignExtend64(BrOffset + 0x800, XLen));
  }
}

unsigned MiniRvccInstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  if (MI.isMetaInstruction())
    return 0;

  unsigned Opcode = MI.getOpcode();

  if (Opcode == TargetOpcode::INLINEASM ||
      Opcode == TargetOpcode::INLINEASM_BR) {
    const MachineFunction &MF = *MI.getParent()->getParent();
    const auto &TM = static_cast<const MiniRvccTargetMachine &>(MF.getTarget());
    return getInlineAsmLength(MI.getOperand(0).getSymbolName(),
                              *TM.getMCAsmInfo());
  }

  if (MI.getParent() && MI.getParent()->getParent()) {
    const auto MF = MI.getMF();
    const auto &TM = static_cast<const MiniRvccTargetMachine &>(MF->getTarget());
    const MCRegisterInfo &MRI = *TM.getMCRegisterInfo();
    const MCSubtargetInfo &STI = *TM.getMCSubtargetInfo();
    const MiniRvccSubtarget &ST = MF->getSubtarget<MiniRvccSubtarget>();
    if (isCompressibleInst(MI, &ST, MRI, STI))
      return 2;
  }
  return get(Opcode).getSize();
}

bool MiniRvccInstrInfo::isAsCheapAsAMove(const MachineInstr &MI) const {
  const unsigned Opcode = MI.getOpcode();
  switch (Opcode) {
  default:
    break;
  case MiniRvcc::FSGNJ_D:
  case MiniRvcc::FSGNJ_S:
  case MiniRvcc::FSGNJ_H:
    // The canonical floating-point move is fsgnj rd, rs, rs.
    return MI.getOperand(1).isReg() && MI.getOperand(2).isReg() &&
           MI.getOperand(1).getReg() == MI.getOperand(2).getReg();
  case MiniRvcc::ADDI:
  case MiniRvcc::ORI:
  case MiniRvcc::XORI:
    return (MI.getOperand(1).isReg() &&
            MI.getOperand(1).getReg() == MiniRvcc::X0) ||
           (MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 0);
  }
  return MI.isAsCheapAsAMove();
}

Optional<DestSourcePair>
MiniRvccInstrInfo::isCopyInstrImpl(const MachineInstr &MI) const {
  if (MI.isMoveReg())
    return DestSourcePair{MI.getOperand(0), MI.getOperand(1)};
  switch (MI.getOpcode()) {
  default:
    break;
  case MiniRvcc::ADDI:
    // Operand 1 can be a frameindex but callers expect registers
    if (MI.getOperand(1).isReg() && MI.getOperand(2).isImm() &&
        MI.getOperand(2).getImm() == 0)
      return DestSourcePair{MI.getOperand(0), MI.getOperand(1)};
    break;
  case MiniRvcc::FSGNJ_D:
  case MiniRvcc::FSGNJ_S:
  case MiniRvcc::FSGNJ_H:
    // The canonical floating-point move is fsgnj rd, rs, rs.
    if (MI.getOperand(1).isReg() && MI.getOperand(2).isReg() &&
        MI.getOperand(1).getReg() == MI.getOperand(2).getReg())
      return DestSourcePair{MI.getOperand(0), MI.getOperand(1)};
    break;
  }
  return None;
}

bool MiniRvccInstrInfo::verifyInstruction(const MachineInstr &MI,
                                       StringRef &ErrInfo) const {
  const MCInstrInfo *MCII = STI.getInstrInfo();
  MCInstrDesc const &Desc = MCII->get(MI.getOpcode());

  for (auto &OI : enumerate(Desc.operands())) {
    unsigned OpType = OI.value().OperandType;
    if (OpType >= MiniRvccOp::OPERAND_FIRST_MiniRvcc_IMM &&
        OpType <= MiniRvccOp::OPERAND_LAST_MiniRvcc_IMM) {
      const MachineOperand &MO = MI.getOperand(OI.index());
      if (MO.isImm()) {
        int64_t Imm = MO.getImm();
        bool Ok;
        switch (OpType) {
        default:
          llvm_unreachable("Unexpected operand type");

          // clang-format off
#define CASE_OPERAND_UIMM(NUM)                                                 \
  case MiniRvccOp::OPERAND_UIMM##NUM:                                             \
    Ok = isUInt<NUM>(Imm);                                                     \
    break;
        CASE_OPERAND_UIMM(2)
        CASE_OPERAND_UIMM(3)
        CASE_OPERAND_UIMM(4)
        CASE_OPERAND_UIMM(5)
        CASE_OPERAND_UIMM(7)
        CASE_OPERAND_UIMM(12)
        CASE_OPERAND_UIMM(20)
          // clang-format on
        case MiniRvccOp::OPERAND_SIMM12:
          Ok = isInt<12>(Imm);
          break;
        case MiniRvccOp::OPERAND_SIMM12_LSB00000:
          Ok = isShiftedInt<7, 5>(Imm);
          break;
        case MiniRvccOp::OPERAND_UIMMLOG2XLEN:
          if (STI.getTargetTriple().isArch64Bit())
            Ok = isUInt<6>(Imm);
          else
            Ok = isUInt<5>(Imm);
          break;
        case MiniRvccOp::OPERAND_RVKRNUM:
          Ok = Imm >= 0 && Imm <= 10;
          break;
        }
        if (!Ok) {
          ErrInfo = "Invalid immediate";
          return false;
        }
      }
    }
  }

  return true;
}

// Return true if get the base operand, byte offset of an instruction and the
// memory width. Width is the size of memory that is being loaded/stored.
bool MiniRvccInstrInfo::getMemOperandWithOffsetWidth(
    const MachineInstr &LdSt, const MachineOperand *&BaseReg, int64_t &Offset,
    unsigned &Width, const TargetRegisterInfo *TRI) const {
  if (!LdSt.mayLoadOrStore())
    return false;

  // Here we assume the standard RISC-V ISA, which uses a base+offset
  // addressing mode. You'll need to relax these conditions to support custom
  // load/stores instructions.
  if (LdSt.getNumExplicitOperands() != 3)
    return false;
  if (!LdSt.getOperand(1).isReg() || !LdSt.getOperand(2).isImm())
    return false;

  if (!LdSt.hasOneMemOperand())
    return false;

  Width = (*LdSt.memoperands_begin())->getSize();
  BaseReg = &LdSt.getOperand(1);
  Offset = LdSt.getOperand(2).getImm();
  return true;
}

bool MiniRvccInstrInfo::areMemAccessesTriviallyDisjoint(
    const MachineInstr &MIa, const MachineInstr &MIb) const {
  assert(MIa.mayLoadOrStore() && "MIa must be a load or store.");
  assert(MIb.mayLoadOrStore() && "MIb must be a load or store.");

  if (MIa.hasUnmodeledSideEffects() || MIb.hasUnmodeledSideEffects() ||
      MIa.hasOrderedMemoryRef() || MIb.hasOrderedMemoryRef())
    return false;

  // Retrieve the base register, offset from the base register and width. Width
  // is the size of memory that is being loaded/stored (e.g. 1, 2, 4).  If
  // base registers are identical, and the offset of a lower memory access +
  // the width doesn't overlap the offset of a higher memory access,
  // then the memory accesses are different.
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();
  const MachineOperand *BaseOpA = nullptr, *BaseOpB = nullptr;
  int64_t OffsetA = 0, OffsetB = 0;
  unsigned int WidthA = 0, WidthB = 0;
  if (getMemOperandWithOffsetWidth(MIa, BaseOpA, OffsetA, WidthA, TRI) &&
      getMemOperandWithOffsetWidth(MIb, BaseOpB, OffsetB, WidthB, TRI)) {
    if (BaseOpA->isIdenticalTo(*BaseOpB)) {
      int LowOffset = std::min(OffsetA, OffsetB);
      int HighOffset = std::max(OffsetA, OffsetB);
      int LowWidth = (LowOffset == OffsetA) ? WidthA : WidthB;
      if (LowOffset + LowWidth <= HighOffset)
        return true;
    }
  }
  return false;
}

std::pair<unsigned, unsigned>
MiniRvccInstrInfo::decomposeMachineOperandsTargetFlags(unsigned TF) const {
  const unsigned Mask = MiniRvccII::MO_DIRECT_FLAG_MASK;
  return std::make_pair(TF & Mask, TF & ~Mask);
}

ArrayRef<std::pair<unsigned, const char *>>
MiniRvccInstrInfo::getSerializableDirectMachineOperandTargetFlags() const {
  using namespace MiniRvccII;
  static const std::pair<unsigned, const char *> TargetFlags[] = {
      {MO_CALL, "minirvcc-call"},
      {MO_PLT, "minirvcc-plt"},
      {MO_LO, "minirvcc-lo"},
      {MO_HI, "minirvcc-hi"},
      {MO_PCREL_LO, "minirvcc-pcrel-lo"},
      {MO_PCREL_HI, "minirvcc-pcrel-hi"},
      {MO_GOT_HI, "minirvcc-got-hi"},
      {MO_TPREL_LO, "minirvcc-tprel-lo"},
      {MO_TPREL_HI, "minirvcc-tprel-hi"},
      {MO_TPREL_ADD, "minirvcc-tprel-add"},
      {MO_TLS_GOT_HI, "minirvcc-tls-got-hi"},
      {MO_TLS_GD_HI, "minirvcc-tls-gd-hi"}};
  return makeArrayRef(TargetFlags);
}
bool MiniRvccInstrInfo::isFunctionSafeToOutlineFrom(
    MachineFunction &MF, bool OutlineFromLinkOnceODRs) const {
  const Function &F = MF.getFunction();

  // Can F be deduplicated by the linker? If it can, don't outline from it.
  if (!OutlineFromLinkOnceODRs && F.hasLinkOnceODRLinkage())
    return false;

  // Don't outline from functions with section markings; the program could
  // expect that all the code is in the named section.
  if (F.hasSection())
    return false;

  // It's safe to outline from MF.
  return true;
}

bool MiniRvccInstrInfo::isMBBSafeToOutlineFrom(MachineBasicBlock &MBB,
                                            unsigned &Flags) const {
  // More accurate safety checking is done in getOutliningCandidateInfo.
  return TargetInstrInfo::isMBBSafeToOutlineFrom(MBB, Flags);
}

// Enum values indicating how an outlined call should be constructed.
enum MachineOutlinerConstructionID {
  MachineOutlinerDefault
};

bool MiniRvccInstrInfo::shouldOutlineFromFunctionByDefault(
    MachineFunction &MF) const {
  return MF.getFunction().hasMinSize();
}

outliner::OutlinedFunction MiniRvccInstrInfo::getOutliningCandidateInfo(
    std::vector<outliner::Candidate> &RepeatedSequenceLocs) const {

  // First we need to filter out candidates where the X5 register (IE t0) can't
  // be used to setup the function call.
  auto CannotInsertCall = [](outliner::Candidate &C) {
    const TargetRegisterInfo *TRI = C.getMF()->getSubtarget().getRegisterInfo();
    return !C.isAvailableAcrossAndOutOfSeq(MiniRvcc::X5, *TRI);
  };

  llvm::erase_if(RepeatedSequenceLocs, CannotInsertCall);

  // If the sequence doesn't have enough candidates left, then we're done.
  if (RepeatedSequenceLocs.size() < 2)
    return outliner::OutlinedFunction();

  unsigned SequenceSize = 0;

  auto I = RepeatedSequenceLocs[0].front();
  auto E = std::next(RepeatedSequenceLocs[0].back());
  for (; I != E; ++I)
    SequenceSize += getInstSizeInBytes(*I);

  // call t0, function = 8 bytes.
  unsigned CallOverhead = 8;
  for (auto &C : RepeatedSequenceLocs)
    C.setCallInfo(MachineOutlinerDefault, CallOverhead);

  // jr t0 = 4 bytes, 2 bytes if compressed instructions are enabled.
  unsigned FrameOverhead = 4;
  if (RepeatedSequenceLocs[0].getMF()->getSubtarget()
          .getFeatureBits()[MiniRvcc::FeatureStdExtC])
    FrameOverhead = 2;

  return outliner::OutlinedFunction(RepeatedSequenceLocs, SequenceSize,
                                    FrameOverhead, MachineOutlinerDefault);
}

outliner::InstrType
MiniRvccInstrInfo::getOutliningType(MachineBasicBlock::iterator &MBBI,
                                 unsigned Flags) const {
  MachineInstr &MI = *MBBI;
  MachineBasicBlock *MBB = MI.getParent();
  const TargetRegisterInfo *TRI =
      MBB->getParent()->getSubtarget().getRegisterInfo();

  // Positions generally can't safely be outlined.
  if (MI.isPosition()) {
    // We can manually strip out CFI instructions later.
    if (MI.isCFIInstruction())
      // If current function has exception handling code, we can't outline &
      // strip these CFI instructions since it may break .eh_frame section
      // needed in unwinding.
      return MI.getMF()->getFunction().needsUnwindTableEntry()
                 ? outliner::InstrType::Illegal
                 : outliner::InstrType::Invisible;

    return outliner::InstrType::Illegal;
  }

  // Don't trust the user to write safe inline assembly.
  if (MI.isInlineAsm())
    return outliner::InstrType::Illegal;

  // We can't outline branches to other basic blocks.
  if (MI.isTerminator() && !MBB->succ_empty())
    return outliner::InstrType::Illegal;

  // We need support for tail calls to outlined functions before return
  // statements can be allowed.
  if (MI.isReturn())
    return outliner::InstrType::Illegal;

  // Don't allow modifying the X5 register which we use for return addresses for
  // these outlined functions.
  if (MI.modifiesRegister(MiniRvcc::X5, TRI) ||
      MI.getDesc().hasImplicitDefOfPhysReg(MiniRvcc::X5))
    return outliner::InstrType::Illegal;

  // Make sure the operands don't reference something unsafe.
  for (const auto &MO : MI.operands())
    if (MO.isMBB() || MO.isBlockAddress() || MO.isCPI() || MO.isJTI())
      return outliner::InstrType::Illegal;

  // Don't allow instructions which won't be materialized to impact outlining
  // analysis.
  if (MI.isMetaInstruction())
    return outliner::InstrType::Invisible;

  return outliner::InstrType::Legal;
}

void MiniRvccInstrInfo::buildOutlinedFrame(
    MachineBasicBlock &MBB, MachineFunction &MF,
    const outliner::OutlinedFunction &OF) const {

  // Strip out any CFI instructions
  bool Changed = true;
  while (Changed) {
    Changed = false;
    auto I = MBB.begin();
    auto E = MBB.end();
    for (; I != E; ++I) {
      if (I->isCFIInstruction()) {
        I->removeFromParent();
        Changed = true;
        break;
      }
    }
  }

  MBB.addLiveIn(MiniRvcc::X5);

  // Add in a return instruction to the end of the outlined frame.
  MBB.insert(MBB.end(), BuildMI(MF, DebugLoc(), get(MiniRvcc::JALR))
      .addReg(MiniRvcc::X0, RegState::Define)
      .addReg(MiniRvcc::X5)
      .addImm(0));
}

MachineBasicBlock::iterator MiniRvccInstrInfo::insertOutlinedCall(
    Module &M, MachineBasicBlock &MBB, MachineBasicBlock::iterator &It,
    MachineFunction &MF, outliner::Candidate &C) const {

  // Add in a call instruction to the outlined function at the given location.
  It = MBB.insert(It,
                  BuildMI(MF, DebugLoc(), get(MiniRvcc::PseudoCALLReg), MiniRvcc::X5)
                      .addGlobalAddress(M.getNamedValue(MF.getName()), 0,
                                        MiniRvccII::MO_CALL));
  return It;
}

// MIR printer helper function to annotate Operands with a comment.
std::string MiniRvccInstrInfo::createMIROperandComment(
    const MachineInstr &MI, const MachineOperand &Op, unsigned OpIdx,
    const TargetRegisterInfo *TRI) const {
  // Print a generic comment for this operand if there is one.
  std::string GenericComment =
      TargetInstrInfo::createMIROperandComment(MI, Op, OpIdx, TRI);
  if (!GenericComment.empty())
    return GenericComment;

  // If not, we must have an immediate operand.
  if (!Op.isImm())
    return std::string();

  std::string Comment;
  raw_string_ostream OS(Comment);

  uint64_t TSFlags = MI.getDesc().TSFlags;

  // Print the full VType operand of vsetvli/vsetivli instructions, and the SEW
  // operand of vector codegen pseudos.
  if ((MI.getOpcode() == MiniRvcc::VSETVLI || MI.getOpcode() == MiniRvcc::VSETIVLI ||
       MI.getOpcode() == MiniRvcc::PseudoVSETVLI ||
       MI.getOpcode() == MiniRvcc::PseudoVSETIVLI ||
       MI.getOpcode() == MiniRvcc::PseudoVSETVLIX0) &&
      OpIdx == 2) {
    unsigned Imm = MI.getOperand(OpIdx).getImm();
    MiniRvccVType::printVType(Imm, OS);
  } else if (MiniRvccII::hasSEWOp(TSFlags)) {
    unsigned NumOperands = MI.getNumExplicitOperands();
    bool HasPolicy = MiniRvccII::hasVecPolicyOp(TSFlags);

    // The SEW operand is before any policy operand.
    if (OpIdx != NumOperands - HasPolicy - 1)
      return std::string();

    unsigned Log2SEW = MI.getOperand(OpIdx).getImm();
    unsigned SEW = Log2SEW ? 1 << Log2SEW : 8;
    assert(MiniRvccVType::isValidSEW(SEW) && "Unexpected SEW");

    OS << "e" << SEW;
  }

  OS.flush();
  return Comment;
}

// clang-format off
#define CASE_VFMA_OPCODE_COMMON(OP, TYPE, LMUL)                                \
  MiniRvcc::PseudoV##OP##_##TYPE##_##LMUL

#define CASE_VFMA_OPCODE_LMULS_M1(OP, TYPE)                                    \
  CASE_VFMA_OPCODE_COMMON(OP, TYPE, M1):                                       \
  case CASE_VFMA_OPCODE_COMMON(OP, TYPE, M2):                                  \
  case CASE_VFMA_OPCODE_COMMON(OP, TYPE, M4):                                  \
  case CASE_VFMA_OPCODE_COMMON(OP, TYPE, M8)

#define CASE_VFMA_OPCODE_LMULS_MF2(OP, TYPE)                                   \
  CASE_VFMA_OPCODE_COMMON(OP, TYPE, MF2):                                      \
  case CASE_VFMA_OPCODE_LMULS_M1(OP, TYPE)

#define CASE_VFMA_OPCODE_LMULS_MF4(OP, TYPE)                                   \
  CASE_VFMA_OPCODE_COMMON(OP, TYPE, MF4):                                      \
  case CASE_VFMA_OPCODE_LMULS_MF2(OP, TYPE)

#define CASE_VFMA_OPCODE_LMULS(OP, TYPE)                                       \
  CASE_VFMA_OPCODE_COMMON(OP, TYPE, MF8):                                      \
  case CASE_VFMA_OPCODE_LMULS_MF4(OP, TYPE)

#define CASE_VFMA_SPLATS(OP)                                                   \
  CASE_VFMA_OPCODE_LMULS_MF4(OP, VF16):                                        \
  case CASE_VFMA_OPCODE_LMULS_MF2(OP, VF32):                                   \
  case CASE_VFMA_OPCODE_LMULS_M1(OP, VF64)
// clang-format on

bool MiniRvccInstrInfo::findCommutedOpIndices(const MachineInstr &MI,
                                           unsigned &SrcOpIdx1,
                                           unsigned &SrcOpIdx2) const {
  const MCInstrDesc &Desc = MI.getDesc();
  if (!Desc.isCommutable())
    return false;

  switch (MI.getOpcode()) {
  case CASE_VFMA_SPLATS(FMADD):
  case CASE_VFMA_SPLATS(FMSUB):
  case CASE_VFMA_SPLATS(FMACC):
  case CASE_VFMA_SPLATS(FMSAC):
  case CASE_VFMA_SPLATS(FNMADD):
  case CASE_VFMA_SPLATS(FNMSUB):
  case CASE_VFMA_SPLATS(FNMACC):
  case CASE_VFMA_SPLATS(FNMSAC):
  case CASE_VFMA_OPCODE_LMULS_MF4(FMACC, VV):
  case CASE_VFMA_OPCODE_LMULS_MF4(FMSAC, VV):
  case CASE_VFMA_OPCODE_LMULS_MF4(FNMACC, VV):
  case CASE_VFMA_OPCODE_LMULS_MF4(FNMSAC, VV):
  case CASE_VFMA_OPCODE_LMULS(MADD, VX):
  case CASE_VFMA_OPCODE_LMULS(NMSUB, VX):
  case CASE_VFMA_OPCODE_LMULS(MACC, VX):
  case CASE_VFMA_OPCODE_LMULS(NMSAC, VX):
  case CASE_VFMA_OPCODE_LMULS(MACC, VV):
  case CASE_VFMA_OPCODE_LMULS(NMSAC, VV): {
    // If the tail policy is undisturbed we can't commute.
    assert(MiniRvccII::hasVecPolicyOp(MI.getDesc().TSFlags));
    if ((MI.getOperand(MI.getNumExplicitOperands() - 1).getImm() & 1) == 0)
      return false;

    // For these instructions we can only swap operand 1 and operand 3 by
    // changing the opcode.
    unsigned CommutableOpIdx1 = 1;
    unsigned CommutableOpIdx2 = 3;
    if (!fixCommutedOpIndices(SrcOpIdx1, SrcOpIdx2, CommutableOpIdx1,
                              CommutableOpIdx2))
      return false;
    return true;
  }
  case CASE_VFMA_OPCODE_LMULS_MF4(FMADD, VV):
  case CASE_VFMA_OPCODE_LMULS_MF4(FMSUB, VV):
  case CASE_VFMA_OPCODE_LMULS_MF4(FNMADD, VV):
  case CASE_VFMA_OPCODE_LMULS_MF4(FNMSUB, VV):
  case CASE_VFMA_OPCODE_LMULS(MADD, VV):
  case CASE_VFMA_OPCODE_LMULS(NMSUB, VV): {
    // If the tail policy is undisturbed we can't commute.
    assert(MiniRvccII::hasVecPolicyOp(MI.getDesc().TSFlags));
    if ((MI.getOperand(MI.getNumExplicitOperands() - 1).getImm() & 1) == 0)
      return false;

    // For these instructions we have more freedom. We can commute with the
    // other multiplicand or with the addend/subtrahend/minuend.

    // Any fixed operand must be from source 1, 2 or 3.
    if (SrcOpIdx1 != CommuteAnyOperandIndex && SrcOpIdx1 > 3)
      return false;
    if (SrcOpIdx2 != CommuteAnyOperandIndex && SrcOpIdx2 > 3)
      return false;

    // It both ops are fixed one must be the tied source.
    if (SrcOpIdx1 != CommuteAnyOperandIndex &&
        SrcOpIdx2 != CommuteAnyOperandIndex && SrcOpIdx1 != 1 && SrcOpIdx2 != 1)
      return false;

    // Look for two different register operands assumed to be commutable
    // regardless of the FMA opcode. The FMA opcode is adjusted later if
    // needed.
    if (SrcOpIdx1 == CommuteAnyOperandIndex ||
        SrcOpIdx2 == CommuteAnyOperandIndex) {
      // At least one of operands to be commuted is not specified and
      // this method is free to choose appropriate commutable operands.
      unsigned CommutableOpIdx1 = SrcOpIdx1;
      if (SrcOpIdx1 == SrcOpIdx2) {
        // Both of operands are not fixed. Set one of commutable
        // operands to the tied source.
        CommutableOpIdx1 = 1;
      } else if (SrcOpIdx1 == CommuteAnyOperandIndex) {
        // Only one of the operands is not fixed.
        CommutableOpIdx1 = SrcOpIdx2;
      }

      // CommutableOpIdx1 is well defined now. Let's choose another commutable
      // operand and assign its index to CommutableOpIdx2.
      unsigned CommutableOpIdx2;
      if (CommutableOpIdx1 != 1) {
        // If we haven't already used the tied source, we must use it now.
        CommutableOpIdx2 = 1;
      } else {
        Register Op1Reg = MI.getOperand(CommutableOpIdx1).getReg();

        // The commuted operands should have different registers.
        // Otherwise, the commute transformation does not change anything and
        // is useless. We use this as a hint to make our decision.
        if (Op1Reg != MI.getOperand(2).getReg())
          CommutableOpIdx2 = 2;
        else
          CommutableOpIdx2 = 3;
      }

      // Assign the found pair of commutable indices to SrcOpIdx1 and
      // SrcOpIdx2 to return those values.
      if (!fixCommutedOpIndices(SrcOpIdx1, SrcOpIdx2, CommutableOpIdx1,
                                CommutableOpIdx2))
        return false;
    }

    return true;
  }
  }

  return TargetInstrInfo::findCommutedOpIndices(MI, SrcOpIdx1, SrcOpIdx2);
}

#define CASE_VFMA_CHANGE_OPCODE_COMMON(OLDOP, NEWOP, TYPE, LMUL)               \
  case MiniRvcc::PseudoV##OLDOP##_##TYPE##_##LMUL:                                \
    Opc = MiniRvcc::PseudoV##NEWOP##_##TYPE##_##LMUL;                             \
    break;

#define CASE_VFMA_CHANGE_OPCODE_LMULS_M1(OLDOP, NEWOP, TYPE)                   \
  CASE_VFMA_CHANGE_OPCODE_COMMON(OLDOP, NEWOP, TYPE, M1)                       \
  CASE_VFMA_CHANGE_OPCODE_COMMON(OLDOP, NEWOP, TYPE, M2)                       \
  CASE_VFMA_CHANGE_OPCODE_COMMON(OLDOP, NEWOP, TYPE, M4)                       \
  CASE_VFMA_CHANGE_OPCODE_COMMON(OLDOP, NEWOP, TYPE, M8)

#define CASE_VFMA_CHANGE_OPCODE_LMULS_MF2(OLDOP, NEWOP, TYPE)                  \
  CASE_VFMA_CHANGE_OPCODE_COMMON(OLDOP, NEWOP, TYPE, MF2)                      \
  CASE_VFMA_CHANGE_OPCODE_LMULS_M1(OLDOP, NEWOP, TYPE)

#define CASE_VFMA_CHANGE_OPCODE_LMULS_MF4(OLDOP, NEWOP, TYPE)                  \
  CASE_VFMA_CHANGE_OPCODE_COMMON(OLDOP, NEWOP, TYPE, MF4)                      \
  CASE_VFMA_CHANGE_OPCODE_LMULS_MF2(OLDOP, NEWOP, TYPE)

#define CASE_VFMA_CHANGE_OPCODE_LMULS(OLDOP, NEWOP, TYPE)                      \
  CASE_VFMA_CHANGE_OPCODE_COMMON(OLDOP, NEWOP, TYPE, MF8)                      \
  CASE_VFMA_CHANGE_OPCODE_LMULS_MF4(OLDOP, NEWOP, TYPE)

#define CASE_VFMA_CHANGE_OPCODE_SPLATS(OLDOP, NEWOP)                           \
  CASE_VFMA_CHANGE_OPCODE_LMULS_MF4(OLDOP, NEWOP, VF16)                        \
  CASE_VFMA_CHANGE_OPCODE_LMULS_MF2(OLDOP, NEWOP, VF32)                        \
  CASE_VFMA_CHANGE_OPCODE_LMULS_M1(OLDOP, NEWOP, VF64)

MachineInstr *MiniRvccInstrInfo::commuteInstructionImpl(MachineInstr &MI,
                                                     bool NewMI,
                                                     unsigned OpIdx1,
                                                     unsigned OpIdx2) const {
  auto cloneIfNew = [NewMI](MachineInstr &MI) -> MachineInstr & {
    if (NewMI)
      return *MI.getParent()->getParent()->CloneMachineInstr(&MI);
    return MI;
  };

  switch (MI.getOpcode()) {
  case CASE_VFMA_SPLATS(FMACC):
  case CASE_VFMA_SPLATS(FMADD):
  case CASE_VFMA_SPLATS(FMSAC):
  case CASE_VFMA_SPLATS(FMSUB):
  case CASE_VFMA_SPLATS(FNMACC):
  case CASE_VFMA_SPLATS(FNMADD):
  case CASE_VFMA_SPLATS(FNMSAC):
  case CASE_VFMA_SPLATS(FNMSUB):
  case CASE_VFMA_OPCODE_LMULS_MF4(FMACC, VV):
  case CASE_VFMA_OPCODE_LMULS_MF4(FMSAC, VV):
  case CASE_VFMA_OPCODE_LMULS_MF4(FNMACC, VV):
  case CASE_VFMA_OPCODE_LMULS_MF4(FNMSAC, VV):
  case CASE_VFMA_OPCODE_LMULS(MADD, VX):
  case CASE_VFMA_OPCODE_LMULS(NMSUB, VX):
  case CASE_VFMA_OPCODE_LMULS(MACC, VX):
  case CASE_VFMA_OPCODE_LMULS(NMSAC, VX):
  case CASE_VFMA_OPCODE_LMULS(MACC, VV):
  case CASE_VFMA_OPCODE_LMULS(NMSAC, VV): {
    // It only make sense to toggle these between clobbering the
    // addend/subtrahend/minuend one of the multiplicands.
    assert((OpIdx1 == 1 || OpIdx2 == 1) && "Unexpected opcode index");
    assert((OpIdx1 == 3 || OpIdx2 == 3) && "Unexpected opcode index");
    unsigned Opc;
    switch (MI.getOpcode()) {
      default:
        llvm_unreachable("Unexpected opcode");
      CASE_VFMA_CHANGE_OPCODE_SPLATS(FMACC, FMADD)
      CASE_VFMA_CHANGE_OPCODE_SPLATS(FMADD, FMACC)
      CASE_VFMA_CHANGE_OPCODE_SPLATS(FMSAC, FMSUB)
      CASE_VFMA_CHANGE_OPCODE_SPLATS(FMSUB, FMSAC)
      CASE_VFMA_CHANGE_OPCODE_SPLATS(FNMACC, FNMADD)
      CASE_VFMA_CHANGE_OPCODE_SPLATS(FNMADD, FNMACC)
      CASE_VFMA_CHANGE_OPCODE_SPLATS(FNMSAC, FNMSUB)
      CASE_VFMA_CHANGE_OPCODE_SPLATS(FNMSUB, FNMSAC)
      CASE_VFMA_CHANGE_OPCODE_LMULS_MF4(FMACC, FMADD, VV)
      CASE_VFMA_CHANGE_OPCODE_LMULS_MF4(FMSAC, FMSUB, VV)
      CASE_VFMA_CHANGE_OPCODE_LMULS_MF4(FNMACC, FNMADD, VV)
      CASE_VFMA_CHANGE_OPCODE_LMULS_MF4(FNMSAC, FNMSUB, VV)
      CASE_VFMA_CHANGE_OPCODE_LMULS(MACC, MADD, VX)
      CASE_VFMA_CHANGE_OPCODE_LMULS(MADD, MACC, VX)
      CASE_VFMA_CHANGE_OPCODE_LMULS(NMSAC, NMSUB, VX)
      CASE_VFMA_CHANGE_OPCODE_LMULS(NMSUB, NMSAC, VX)
      CASE_VFMA_CHANGE_OPCODE_LMULS(MACC, MADD, VV)
      CASE_VFMA_CHANGE_OPCODE_LMULS(NMSAC, NMSUB, VV)
    }

    auto &WorkingMI = cloneIfNew(MI);
    WorkingMI.setDesc(get(Opc));
    return TargetInstrInfo::commuteInstructionImpl(WorkingMI, /*NewMI=*/false,
                                                   OpIdx1, OpIdx2);
  }
  case CASE_VFMA_OPCODE_LMULS_MF4(FMADD, VV):
  case CASE_VFMA_OPCODE_LMULS_MF4(FMSUB, VV):
  case CASE_VFMA_OPCODE_LMULS_MF4(FNMADD, VV):
  case CASE_VFMA_OPCODE_LMULS_MF4(FNMSUB, VV):
  case CASE_VFMA_OPCODE_LMULS(MADD, VV):
  case CASE_VFMA_OPCODE_LMULS(NMSUB, VV): {
    assert((OpIdx1 == 1 || OpIdx2 == 1) && "Unexpected opcode index");
    // If one of the operands, is the addend we need to change opcode.
    // Otherwise we're just swapping 2 of the multiplicands.
    if (OpIdx1 == 3 || OpIdx2 == 3) {
      unsigned Opc;
      switch (MI.getOpcode()) {
        default:
          llvm_unreachable("Unexpected opcode");
        CASE_VFMA_CHANGE_OPCODE_LMULS_MF4(FMADD, FMACC, VV)
        CASE_VFMA_CHANGE_OPCODE_LMULS_MF4(FMSUB, FMSAC, VV)
        CASE_VFMA_CHANGE_OPCODE_LMULS_MF4(FNMADD, FNMACC, VV)
        CASE_VFMA_CHANGE_OPCODE_LMULS_MF4(FNMSUB, FNMSAC, VV)
        CASE_VFMA_CHANGE_OPCODE_LMULS(MADD, MACC, VV)
        CASE_VFMA_CHANGE_OPCODE_LMULS(NMSUB, NMSAC, VV)
      }

      auto &WorkingMI = cloneIfNew(MI);
      WorkingMI.setDesc(get(Opc));
      return TargetInstrInfo::commuteInstructionImpl(WorkingMI, /*NewMI=*/false,
                                                     OpIdx1, OpIdx2);
    }
    // Let the default code handle it.
    break;
  }
  }

  return TargetInstrInfo::commuteInstructionImpl(MI, NewMI, OpIdx1, OpIdx2);
}

#undef CASE_VFMA_CHANGE_OPCODE_SPLATS
#undef CASE_VFMA_CHANGE_OPCODE_LMULS
#undef CASE_VFMA_CHANGE_OPCODE_COMMON
#undef CASE_VFMA_SPLATS
#undef CASE_VFMA_OPCODE_LMULS
#undef CASE_VFMA_OPCODE_COMMON

// clang-format off
#define CASE_WIDEOP_OPCODE_COMMON(OP, LMUL)                                    \
  MiniRvcc::PseudoV##OP##_##LMUL##_TIED

#define CASE_WIDEOP_OPCODE_LMULS_MF4(OP)                                       \
  CASE_WIDEOP_OPCODE_COMMON(OP, MF4):                                          \
  case CASE_WIDEOP_OPCODE_COMMON(OP, MF2):                                     \
  case CASE_WIDEOP_OPCODE_COMMON(OP, M1):                                      \
  case CASE_WIDEOP_OPCODE_COMMON(OP, M2):                                      \
  case CASE_WIDEOP_OPCODE_COMMON(OP, M4)

#define CASE_WIDEOP_OPCODE_LMULS(OP)                                           \
  CASE_WIDEOP_OPCODE_COMMON(OP, MF8):                                          \
  case CASE_WIDEOP_OPCODE_LMULS_MF4(OP)
// clang-format on

#define CASE_WIDEOP_CHANGE_OPCODE_COMMON(OP, LMUL)                             \
  case MiniRvcc::PseudoV##OP##_##LMUL##_TIED:                                     \
    NewOpc = MiniRvcc::PseudoV##OP##_##LMUL;                                      \
    break;

#define CASE_WIDEOP_CHANGE_OPCODE_LMULS_MF4(OP)                                 \
  CASE_WIDEOP_CHANGE_OPCODE_COMMON(OP, MF4)                                    \
  CASE_WIDEOP_CHANGE_OPCODE_COMMON(OP, MF2)                                    \
  CASE_WIDEOP_CHANGE_OPCODE_COMMON(OP, M1)                                     \
  CASE_WIDEOP_CHANGE_OPCODE_COMMON(OP, M2)                                     \
  CASE_WIDEOP_CHANGE_OPCODE_COMMON(OP, M4)

#define CASE_WIDEOP_CHANGE_OPCODE_LMULS(OP)                                    \
  CASE_WIDEOP_CHANGE_OPCODE_COMMON(OP, MF8)                                    \
  CASE_WIDEOP_CHANGE_OPCODE_LMULS_MF4(OP)

MachineInstr *MiniRvccInstrInfo::convertToThreeAddress(MachineInstr &MI,
                                                    LiveVariables *LV,
                                                    LiveIntervals *LIS) const {
  switch (MI.getOpcode()) {
  default:
    break;
  case CASE_WIDEOP_OPCODE_LMULS_MF4(FWADD_WV):
  case CASE_WIDEOP_OPCODE_LMULS_MF4(FWSUB_WV):
  case CASE_WIDEOP_OPCODE_LMULS(WADD_WV):
  case CASE_WIDEOP_OPCODE_LMULS(WADDU_WV):
  case CASE_WIDEOP_OPCODE_LMULS(WSUB_WV):
  case CASE_WIDEOP_OPCODE_LMULS(WSUBU_WV): {
    // If the tail policy is undisturbed we can't convert.
    assert(MiniRvccII::hasVecPolicyOp(MI.getDesc().TSFlags) &&
           MI.getNumExplicitOperands() == 6);
    if ((MI.getOperand(5).getImm() & 1) == 0)
      return nullptr;

    // clang-format off
    unsigned NewOpc;
    switch (MI.getOpcode()) {
    default:
      llvm_unreachable("Unexpected opcode");
    CASE_WIDEOP_CHANGE_OPCODE_LMULS_MF4(FWADD_WV)
    CASE_WIDEOP_CHANGE_OPCODE_LMULS_MF4(FWSUB_WV)
    CASE_WIDEOP_CHANGE_OPCODE_LMULS(WADD_WV)
    CASE_WIDEOP_CHANGE_OPCODE_LMULS(WADDU_WV)
    CASE_WIDEOP_CHANGE_OPCODE_LMULS(WSUB_WV)
    CASE_WIDEOP_CHANGE_OPCODE_LMULS(WSUBU_WV)
    }
    // clang-format on

    MachineBasicBlock &MBB = *MI.getParent();
    MachineInstrBuilder MIB = BuildMI(MBB, MI, MI.getDebugLoc(), get(NewOpc))
                                  .add(MI.getOperand(0))
                                  .add(MI.getOperand(1))
                                  .add(MI.getOperand(2))
                                  .add(MI.getOperand(3))
                                  .add(MI.getOperand(4));
    MIB.copyImplicitOps(MI);

    if (LV) {
      unsigned NumOps = MI.getNumOperands();
      for (unsigned I = 1; I < NumOps; ++I) {
        MachineOperand &Op = MI.getOperand(I);
        if (Op.isReg() && Op.isKill())
          LV->replaceKillInstruction(Op.getReg(), MI, *MIB);
      }
    }

    if (LIS) {
      SlotIndex Idx = LIS->ReplaceMachineInstrInMaps(MI, *MIB);

      if (MI.getOperand(0).isEarlyClobber()) {
        // Use operand 1 was tied to early-clobber def operand 0, so its live
        // interval could have ended at an early-clobber slot. Now they are not
        // tied we need to update it to the normal register slot.
        LiveInterval &LI = LIS->getInterval(MI.getOperand(1).getReg());
        LiveRange::Segment *S = LI.getSegmentContaining(Idx);
        if (S->end == Idx.getRegSlot(true))
          S->end = Idx.getRegSlot();
      }
    }

    return MIB;
  }
  }

  return nullptr;
}

#undef CASE_WIDEOP_CHANGE_OPCODE_LMULS
#undef CASE_WIDEOP_CHANGE_OPCODE_COMMON
#undef CASE_WIDEOP_OPCODE_LMULS
#undef CASE_WIDEOP_OPCODE_COMMON

Register MiniRvccInstrInfo::getVLENFactoredAmount(MachineFunction &MF,
                                               MachineBasicBlock &MBB,
                                               MachineBasicBlock::iterator II,
                                               const DebugLoc &DL,
                                               int64_t Amount,
                                               MachineInstr::MIFlag Flag) const {
  assert(Amount > 0 && "There is no need to get VLEN scaled value.");
  assert(Amount % 8 == 0 &&
         "Reserve the stack by the multiple of one vector size.");

  MachineRegisterInfo &MRI = MF.getRegInfo();
  int64_t NumOfVReg = Amount / 8;

  Register VL = MRI.createVirtualRegister(&MiniRvcc::GPRRegClass);
  BuildMI(MBB, II, DL, get(MiniRvcc::PseudoReadVLENB), VL)
    .setMIFlag(Flag);
  assert(isInt<32>(NumOfVReg) &&
         "Expect the number of vector registers within 32-bits.");
  if (isPowerOf2_32(NumOfVReg)) {
    uint32_t ShiftAmount = Log2_32(NumOfVReg);
    if (ShiftAmount == 0)
      return VL;
    BuildMI(MBB, II, DL, get(MiniRvcc::SLLI), VL)
        .addReg(VL, RegState::Kill)
        .addImm(ShiftAmount)
        .setMIFlag(Flag);
  } else if (STI.hasStdExtZba() &&
             ((NumOfVReg % 3 == 0 && isPowerOf2_64(NumOfVReg / 3)) ||
              (NumOfVReg % 5 == 0 && isPowerOf2_64(NumOfVReg / 5)) ||
              (NumOfVReg % 9 == 0 && isPowerOf2_64(NumOfVReg / 9)))) {
    // We can use Zba SHXADD+SLLI instructions for multiply in some cases.
    unsigned Opc;
    uint32_t ShiftAmount;
    if (NumOfVReg % 9 == 0) {
      Opc = MiniRvcc::SH3ADD;
      ShiftAmount = Log2_64(NumOfVReg / 9);
    } else if (NumOfVReg % 5 == 0) {
      Opc = MiniRvcc::SH2ADD;
      ShiftAmount = Log2_64(NumOfVReg / 5);
    } else if (NumOfVReg % 3 == 0) {
      Opc = MiniRvcc::SH1ADD;
      ShiftAmount = Log2_64(NumOfVReg / 3);
    } else {
      llvm_unreachable("Unexpected number of vregs");
    }
    if (ShiftAmount)
      BuildMI(MBB, II, DL, get(MiniRvcc::SLLI), VL)
          .addReg(VL, RegState::Kill)
          .addImm(ShiftAmount)
          .setMIFlag(Flag);
    BuildMI(MBB, II, DL, get(Opc), VL)
        .addReg(VL, RegState::Kill)
        .addReg(VL)
        .setMIFlag(Flag);
  } else if (isPowerOf2_32(NumOfVReg - 1)) {
    Register ScaledRegister = MRI.createVirtualRegister(&MiniRvcc::GPRRegClass);
    uint32_t ShiftAmount = Log2_32(NumOfVReg - 1);
    BuildMI(MBB, II, DL, get(MiniRvcc::SLLI), ScaledRegister)
        .addReg(VL)
        .addImm(ShiftAmount)
        .setMIFlag(Flag);
    BuildMI(MBB, II, DL, get(MiniRvcc::ADD), VL)
        .addReg(ScaledRegister, RegState::Kill)
        .addReg(VL, RegState::Kill)
        .setMIFlag(Flag);
  } else if (isPowerOf2_32(NumOfVReg + 1)) {
    Register ScaledRegister = MRI.createVirtualRegister(&MiniRvcc::GPRRegClass);
    uint32_t ShiftAmount = Log2_32(NumOfVReg + 1);
    BuildMI(MBB, II, DL, get(MiniRvcc::SLLI), ScaledRegister)
        .addReg(VL)
        .addImm(ShiftAmount)
        .setMIFlag(Flag);
    BuildMI(MBB, II, DL, get(MiniRvcc::SUB), VL)
        .addReg(ScaledRegister, RegState::Kill)
        .addReg(VL, RegState::Kill)
        .setMIFlag(Flag);
  } else {
    Register N = MRI.createVirtualRegister(&MiniRvcc::GPRRegClass);
    movImm(MBB, II, DL, N, NumOfVReg, Flag);
    if (!STI.hasStdExtM() && !STI.hasStdExtZmmul())
      MF.getFunction().getContext().diagnose(DiagnosticInfoUnsupported{
          MF.getFunction(),
          "M- or Zmmul-extension must be enabled to calculate the vscaled size/"
          "offset."});
    BuildMI(MBB, II, DL, get(MiniRvcc::MUL), VL)
        .addReg(VL, RegState::Kill)
        .addReg(N, RegState::Kill)
        .setMIFlag(Flag);
  }

  return VL;
}

// Returns true if this is the sext.w pattern, addiw rd, rs1, 0.
bool MiniRvcc::isSEXT_W(const MachineInstr &MI) {
  return MI.getOpcode() == MiniRvcc::ADDIW && MI.getOperand(1).isReg() &&
         MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 0;
}

// Returns true if this is the zext.w pattern, adduw rd, rs1, x0.
bool MiniRvcc::isZEXT_W(const MachineInstr &MI) {
  return MI.getOpcode() == MiniRvcc::ADD_UW && MI.getOperand(1).isReg() &&
         MI.getOperand(2).isReg() && MI.getOperand(2).getReg() == MiniRvcc::X0;
}

// Returns true if this is the zext.b pattern, andi rd, rs1, 255.
bool MiniRvcc::isZEXT_B(const MachineInstr &MI) {
  return MI.getOpcode() == MiniRvcc::ANDI && MI.getOperand(1).isReg() &&
         MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 255;
}

static bool isRVVWholeLoadStore(unsigned Opcode) {
  switch (Opcode) {
  default:
    return false;
  case MiniRvcc::VS1R_V:
  case MiniRvcc::VS2R_V:
  case MiniRvcc::VS4R_V:
  case MiniRvcc::VS8R_V:
  case MiniRvcc::VL1RE8_V:
  case MiniRvcc::VL2RE8_V:
  case MiniRvcc::VL4RE8_V:
  case MiniRvcc::VL8RE8_V:
  case MiniRvcc::VL1RE16_V:
  case MiniRvcc::VL2RE16_V:
  case MiniRvcc::VL4RE16_V:
  case MiniRvcc::VL8RE16_V:
  case MiniRvcc::VL1RE32_V:
  case MiniRvcc::VL2RE32_V:
  case MiniRvcc::VL4RE32_V:
  case MiniRvcc::VL8RE32_V:
  case MiniRvcc::VL1RE64_V:
  case MiniRvcc::VL2RE64_V:
  case MiniRvcc::VL4RE64_V:
  case MiniRvcc::VL8RE64_V:
    return true;
  }
}

bool MiniRvcc::isRVVSpill(const MachineInstr &MI) {
  // RVV lacks any support for immediate addressing for stack addresses, so be
  // conservative.
  unsigned Opcode = MI.getOpcode();
  if (!MiniRvccVPseudosTable::getPseudoInfo(Opcode) &&
      !isRVVWholeLoadStore(Opcode) && !isRVVSpillForZvlsseg(Opcode))
    return false;
  return true;
}

Optional<std::pair<unsigned, unsigned>>
MiniRvcc::isRVVSpillForZvlsseg(unsigned Opcode) {
  switch (Opcode) {
  default:
    return None;
  case MiniRvcc::PseudoVSPILL2_M1:
  case MiniRvcc::PseudoVRELOAD2_M1:
    return std::make_pair(2u, 1u);
  case MiniRvcc::PseudoVSPILL2_M2:
  case MiniRvcc::PseudoVRELOAD2_M2:
    return std::make_pair(2u, 2u);
  case MiniRvcc::PseudoVSPILL2_M4:
  case MiniRvcc::PseudoVRELOAD2_M4:
    return std::make_pair(2u, 4u);
  case MiniRvcc::PseudoVSPILL3_M1:
  case MiniRvcc::PseudoVRELOAD3_M1:
    return std::make_pair(3u, 1u);
  case MiniRvcc::PseudoVSPILL3_M2:
  case MiniRvcc::PseudoVRELOAD3_M2:
    return std::make_pair(3u, 2u);
  case MiniRvcc::PseudoVSPILL4_M1:
  case MiniRvcc::PseudoVRELOAD4_M1:
    return std::make_pair(4u, 1u);
  case MiniRvcc::PseudoVSPILL4_M2:
  case MiniRvcc::PseudoVRELOAD4_M2:
    return std::make_pair(4u, 2u);
  case MiniRvcc::PseudoVSPILL5_M1:
  case MiniRvcc::PseudoVRELOAD5_M1:
    return std::make_pair(5u, 1u);
  case MiniRvcc::PseudoVSPILL6_M1:
  case MiniRvcc::PseudoVRELOAD6_M1:
    return std::make_pair(6u, 1u);
  case MiniRvcc::PseudoVSPILL7_M1:
  case MiniRvcc::PseudoVRELOAD7_M1:
    return std::make_pair(7u, 1u);
  case MiniRvcc::PseudoVSPILL8_M1:
  case MiniRvcc::PseudoVRELOAD8_M1:
    return std::make_pair(8u, 1u);
  }
}

bool MiniRvcc::isFaultFirstLoad(const MachineInstr &MI) {
  return MI.getNumExplicitDefs() == 2 && MI.modifiesRegister(MiniRvcc::VL) &&
         !MI.isInlineAsm();
}
