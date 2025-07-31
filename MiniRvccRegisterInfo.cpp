//===-- MiniRvccRegisterInfo.cpp - MiniRvcc Register Information ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the MiniRvcc implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "MiniRvccRegisterInfo.h"
#include "MiniRvcc.h"
#include "MiniRvccMachineFunctionInfo.h"
#include "MiniRvccSubtarget.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/ErrorHandling.h"

#define GET_REGINFO_TARGET_DESC
#include "MiniRvccGenRegisterInfo.inc"

using namespace llvm;


MiniRvccRegisterInfo::MiniRvccRegisterInfo(unsigned HwMode)
    : MiniRvccGenRegisterInfo(MiniRvcc::X1, /*DwarfFlavour*/0, /*EHFlavor*/0,
                              /*PC*/0, HwMode) {}


/// 呼び出し側で保存すべきレジスタの一覧を返す
const MCPhysReg *
MiniRvccRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
    // サブターゲット情報を取得
    auto &Subtarget = MF->getSubtarget<MiniRvccSubtarget>();

    /*if (MF->getFunction().getCallingConv() == CallingConv::GHC)
        return CSR_NoRegs_SaveList;
    if (MF->getFunction().hasFnAttribute("interrupt")) {
        if (Subtarget.hasStdExtD())
            return CSR_XLEN_F64_Interrupt_SaveList;
        if (Subtarget.hasStdExtF())
            return CSR_XLEN_F32_Interrupt_SaveList;
        return CSR_Interrupt_SaveList;
    }*/

    // CallingConv.tdで定義した保存レジスタリストを返す
    return CSR_ILP32;

    /*
    // ABI に応じた保存レジスタリストを返す
    switch (Subtarget.getTargetABI()) {
    default:
        llvm_unreachable("Unrecognized ABI");
    case MiniRvccABI::ABI_ILP32:
    case MiniRvccABI::ABI_LP64:
        return CSR_ILP32_LP64_SaveList;
    case MiniRvccABI::ABI_ILP32F:
    case MiniRvccABI::ABI_LP64F:
        return CSR_ILP32F_LP64F_SaveList;
    case MiniRvccABI::ABI_ILP32D:
    case MiniRvccABI::ABI_LP64D:
        return CSR_ILP32D_LP64D_SaveList;
    }*/
}


/// レジスタ割当の対象外とするレジスタを予約指定する
BitVector MiniRvccRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
    const MiniRvccFrameLowering *TFI = getFrameLowering(MF);
    BitVector Reserved(getNumRegs());

    /*// Mark any registers requested to be reserved as such
    for (size_t Reg = 0; Reg < getNumRegs(); Reg++) {
        if (MF.getSubtarget<RISCVSubtarget>().isRegisterReservedByUser(Reg))
            markSuperRegs(Reserved, Reg);
    }*/

    // 以下を使用禁止レジスタとして予約
    markSuperRegs(Reserved, RISCV::X0); // zero
    markSuperRegs(Reserved, RISCV::X2); // sp
    markSuperRegs(Reserved, RISCV::X3); // gp
    markSuperRegs(Reserved, RISCV::X4); // tp

    if (TFI->hasFP(MF))
        markSuperRegs(Reserved, RISCV::X8); // fp

    /*
    // Reserve the base register if we need to realign the stack and allocate
    // variable-sized objects at runtime.
    if (TFI->hasBP(MF))
        markSuperRegs(Reserved, RISCVABI::getBPReg()); // bp

    // V registers for code generation. We handle them manually.
    markSuperRegs(Reserved, RISCV::VL);
    markSuperRegs(Reserved, RISCV::VTYPE);
    markSuperRegs(Reserved, RISCV::VXSAT);
    markSuperRegs(Reserved, RISCV::VXRM);
    markSuperRegs(Reserved, RISCV::VLENB); // vlenb (constant)

    // Floating point environment registers.
    markSuperRegs(Reserved, RISCV::FRM);
    markSuperRegs(Reserved, RISCV::FFLAGS);
    assert(checkAllSuperRegsMarked(Reserved));
    */

    return Reserved;
}

/*bool RISCVRegisterInfo::isAsmClobberable(const MachineFunction &MF,
                                         MCRegister PhysReg) const {
  return !MF.getSubtarget<RISCVSubtarget>().isRegisterReservedByUser(PhysReg);
}*/

/*bool RISCVRegisterInfo::isConstantPhysReg(MCRegister PhysReg) const {
  return PhysReg == RISCV::X0 || PhysReg == RISCV::VLENB;
}*/

/*const uint32_t *RISCVRegisterInfo::getNoPreservedMask() const {
  return CSR_NoRegs_RegMask;
}*/

#if 0
// Frame indexes representing locations of CSRs which are given a fixed location
// b・＊save/restore libcalls.
static const std::pair<unsigned, int> FixedCSRFIMap[] = {
  {/*ra*/  RISCV::X1,   -1},
  {/*s0*/  RISCV::X8,   -2},
  {/*s1*/  RISCV::X9,   -3},
  {/*s2*/  RISCV::X18,  -4},
  {/*s3*/  RISCV::X19,  -5},
  {/*s4*/  RISCV::X20,  -6},
  {/*s5*/  RISCV::X21,  -7},
  {/*s6*/  RISCV::X22,  -8},
  {/*s7*/  RISCV::X23,  -9},
  {/*s8*/  RISCV::X24,  -10},
  {/*s9*/  RISCV::X25,  -11},
  {/*s10*/ RISCV::X26,  -12},
  {/*s11*/ RISCV::X27,  -13}
};
#endif

/*bool RISCVRegisterInfo::hasReservedSpillSlot(const MachineFunction &MF,
                                             Register Reg,
                                             int &FrameIdx) const {
  const auto *RVFI = MF.getInfo<RISCVMachineFunctionInfo>();
  if (!RVFI->useSaveRestoreLibCalls(MF))
    return false;

  const auto *FII =
      llvm::find_if(FixedCSRFIMap, [&](auto P) { return P.first == Reg; });
  if (FII == std::end(FixedCSRFIMap))
    return false;

  FrameIdx = FII->second;
  return true;
}*/


/// フレームインデックスを実アドレス（SPやFPを基準としたオフセット）に置き換える
void MiniRvccRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                               int SPAdj, unsigned FIOperandNum,
                                               RegScavenger *RS) const {
    assert(SPAdj == 0 && "Unexpected non-zero SPAdj value");

    MachineInstr &MI = *II;
    MachineFunction &MF = *MI.getParent()->getParent();
    MachineRegisterInfo &MRI = MF.getRegInfo();
    const MiniRvccInstrInfo *TII = MF.getSubtarget<MiniRvccSubtarget>().getInstrInfo();
    DebugLoc DL = MI.getDebugLoc();

    // フレームインデックス取得
    int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
    Register FrameReg;

    // フレームレジスタとオフセットを取得（通常はspまたはfp）
    StackOffset Offset =
        getFrameLowering(MF)->getFrameIndexReference(MF, FrameIndex, FrameReg);

    /*bool IsRVVSpill = RISCV::isRVVSpill(MI);
    if (!IsRVVSpill)
        Offset += StackOffset::getFixed(MI.getOperand(FIOperandNum + 1).getImm());

    if (!isInt<32>(Offset.getFixed())) {
        report_fatal_error("Frame offsets outside of the signed 32-bit range not supported");
    }*/

    MachineBasicBlock &MBB = *MI.getParent();
    //bool FrameRegIsKill = false;

    /*// If required, pre-compute the scalable factor amount which will be used in
    // later offset computation. Since this sequence requires up to two scratch
    // registers -- after which one is made free -- this grants us better
    // scavenging of scratch registers as only up to two are live at one time,
    // rather than three.
    Register ScalableFactorRegister;
    unsigned ScalableAdjOpc = RISCV::ADD;
    if (Offset.getScalable()) {
        int64_t ScalableValue = Offset.getScalable();
        if (ScalableValue < 0) {
            ScalableValue = -ScalableValue;
            ScalableAdjOpc = RISCV::SUB;
        }
        // 1. Get vlenb && multiply vlen with the number of vector registers.
        ScalableFactorRegister =
            TII->getVLENFactoredAmount(MF, MBB, II, DL, ScalableValue);
    }*/

    // オフセットが12ビットに収まらない場合は、即値として使用できないので
    // 一時レジスタにロードして加算する
    if (!isInt<12>(Offset.getFixed())) {
        // The offset won't fit in an immediate, so use a scratch register instead
        // Modify Offset and FrameReg appropriately
        Register ScratchReg = MRI.createVirtualRegister(&MiniRvcc::GPRRegClass);
        // 即値をScratchRegにロード
        TII->movImm(*MI.getParent(), II, DL, ScratchReg, Offset.getFixed());
        //if (MI.getOpcode() == RISCV::ADDI && !Offset.getScalable()) {
            BuildMI(*MI.getParent(), II, DL, TII->get(MiniRvcc::ADD), 
                    MI.getOperand(0).getReg())
                .addReg(FrameReg)
                .addReg(ScratchReg, RegState::Kill);
            MI.eraseFromParent();
            return;
        //}
        //BuildMI(MBB, II, DL, TII->get(RISCV::ADD), ScratchReg)
        //    .addReg(FrameReg)
        //    .addReg(ScratchReg, RegState::Kill);
        //Offset = StackOffset::get(0, Offset.getScalable());
        //FrameReg = ScratchReg;
        //FrameRegIsKill = true;
    }

    //if (!Offset.getScalable()) {
        // Offset = (fixed offset, 0)

        // 即値が12ビットに収まる場合は、そのままFrameRegとオフセットで置き換える
        MI.getOperand(FIOperandNum)
            .ChangeToRegister(FrameReg, false, false, false);
        //if (!IsRVVSpill)
            MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset.getFixed());
        //else {
        //    if (Offset.getFixed()) {
        //        Register ScratchReg = MRI.createVirtualRegister(&RISCV::GPRRegClass);
        //        BuildMI(MBB, II, DL, TII->get(RISCV::ADDI), ScratchReg)
        //            .addReg(FrameReg, getKillRegState(FrameRegIsKill))
        //            .addImm(Offset.getFixed());
        //        MI.getOperand(FIOperandNum)
        //            .ChangeToRegister(ScratchReg, false, false, true);
        //    }
        //}
    //} else {
    //    // Offset = (fixed offset, scalable offset)
    //    // Step 1, the scalable offset, has already been computed.
    //    assert(ScalableFactorRegister &&
    //       "Expected pre-computation of scalable factor in earlier step");

    //    // 2. Calculate address: FrameReg + result of multiply
    //    if (MI.getOpcode() == RISCV::ADDI && !Offset.getFixed()) {
    //        BuildMI(MBB, II, DL, TII->get(ScalableAdjOpc), MI.getOperand(0).getReg())
    //            .addReg(FrameReg, getKillRegState(FrameRegIsKill))
    //            .addReg(ScalableFactorRegister, RegState::Kill);
    //        MI.eraseFromParent();
    //        return;
    //    }
    //    Register VL = MRI.createVirtualRegister(&RISCV::GPRRegClass);
    //    BuildMI(MBB, II, DL, TII->get(ScalableAdjOpc), VL)
    //        .addReg(FrameReg, getKillRegState(FrameRegIsKill))
    //        .addReg(ScalableFactorRegister, RegState::Kill);

    //    if (IsRVVSpill && Offset.getFixed()) {
    //        // Scalable load/store has no immediate argument. We need to add the
    //        // fixed part into the load/store base address.
    //        BuildMI(MBB, II, DL, TII->get(RISCV::ADDI), VL)
    //            .addReg(VL)
    //            .addImm(Offset.getFixed());
    //    }

    //    // 3. Replace address register with calculated address register
    //    MI.getOperand(FIOperandNum).ChangeToRegister(VL, false, false, true);
    //    if (!IsRVVSpill)
    //        MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset.getFixed());
    //}

    //auto ZvlssegInfo = RISCV::isRVVSpillForZvlsseg(MI.getOpcode());
    //if (ZvlssegInfo) {
    //    Register VL = MRI.createVirtualRegister(&RISCV::GPRRegClass);
    //    BuildMI(MBB, II, DL, TII->get(RISCV::PseudoReadVLENB), VL);
    //    uint32_t ShiftAmount = Log2_32(ZvlssegInfo->second);
    //    if (ShiftAmount != 0)
    //    BuildMI(MBB, II, DL, TII->get(RISCV::SLLI), VL)
    //        .addReg(VL)
    //        .addImm(ShiftAmount);
    //    // The last argument of pseudo spilling opcode for zvlsseg is the length of
    //    // one element of zvlsseg types. For example, for vint32m2x2_t, it will be
    //    // the length of vint32m2_t.
    //    MI.getOperand(FIOperandNum + 1).ChangeToRegister(VL, /*isDef=*/false);
    //}
}


/// 関数のフレームレジスタを返す
/// - フレームレジスタは、スタックフレームにおけるローカル変数等の
///   アクセスを容易にするために使用される。
/// - 通常はフレームポインタ (fp = X8)、省略可能な場合はスタックポインタ (sp = X2) を使用。
Register MiniRvccRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = getFrameLowering(MF);
  // hasFP = フレームポインタを使用するかどうかの判定
  return TFI->hasFP(MF) ? MiniRvcc::X8 : MiniRvcc::X2;
}

/// 呼び出し規約と ABI に基づいて、関数呼び出し時に保存すべきレジスタのマスクを返す
const uint32_t *
MiniRvccRegisterInfo::getCallPreservedMask(const MachineFunction & MF,
                                           CallingConv::ID CC) const {
    // 対象関数のサブターゲット（CPU情報など）を取得
    auto &Subtarget = MF.getSubtarget<MiniRvccSubtarget>();

    // GHC (Glasgow Haskell Compiler) 用の呼び出し規約では、レジスタを保存する必要がない
    if (CC == CallingConv::GHC)
        return CSR_NoRegs_RegMask;

    // ABI に応じて保存すべきレジスタマスクを選択
    /*switch (Subtarget.getTargetABI()) {
    default:
        llvm_unreachable("Unrecognized ABI");
    case RISCVABI::ABI_ILP32:
    //case RISCVABI::ABI_LP64:
        return CSR_ILP32_LP64_RegMask;
    //case RISCVABI::ABI_ILP32F:
    //case RISCVABI::ABI_LP64F:
    //    return CSR_ILP32F_LP64F_RegMask;
    //case RISCVABI::ABI_ILP32D:
    //case RISCVABI::ABI_LP64D:
    //    return CSR_ILP32D_LP64D_RegMask;
    }*/

    return CSR_ILP32_RegMask;
}

//const TargetRegisterClass *
//RISCVRegisterInfo::getLargestLegalSuperClass(const TargetRegisterClass *RC,
//                                             const MachineFunction &) const {
//  if (RC == &RISCV::VMV0RegClass)
//    return &RISCV::VRRegClass;
//  return RC;
//}

/*void RISCVRegisterInfo::getOffsetOpcodes(const StackOffset &Offset,
                                         SmallVectorImpl<uint64_t> &Ops) const {
  // VLENB is the length of a vector register in bytes. We use <vscale x 8 x i8>
  // to represent one vector register. The dwarf offset is
  // VLENB * scalable_offset / 8.
  assert(Offset.getScalable() % 8 == 0 && "Invalid frame offset");

  // Add fixed-sized offset using existing DIExpression interface.
  DIExpression::appendOffset(Ops, Offset.getFixed());

  unsigned VLENB = getDwarfRegNum(RISCV::VLENB, true);
  int64_t VLENBSized = Offset.getScalable() / 8;
  if (VLENBSized > 0) {
    Ops.push_back(dwarf::DW_OP_constu);
    Ops.push_back(VLENBSized);
    Ops.append({dwarf::DW_OP_bregx, VLENB, 0ULL});
    Ops.push_back(dwarf::DW_OP_mul);
    Ops.push_back(dwarf::DW_OP_plus);
  } else if (VLENBSized < 0) {
    Ops.push_back(dwarf::DW_OP_constu);
    Ops.push_back(-VLENBSized);
    Ops.append({dwarf::DW_OP_bregx, VLENB, 0ULL});
    Ops.push_back(dwarf::DW_OP_mul);
    Ops.push_back(dwarf::DW_OP_minus);
  }
}*/

//unsigned
//RISCVRegisterInfo::getRegisterCostTableIndex(const MachineFunction &MF) const {
//  return MF.getSubtarget<RISCVSubtarget>().hasStdExtC() ? 1 : 0;
//}
