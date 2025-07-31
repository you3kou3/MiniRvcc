//===-- MiniRvccISelLowering.h - MiniRvcc DAG Lowering Interface ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that MiniRvcc uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MINIRVCC_MINIRVCCISELLOWERING_H
#define LLVM_LIB_TARGET_MINIRVCC_MINIRVCCISELLOWERING_H

#include "MiniRvcc.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {
class MiniRvccSubtarget;
struct MiniRvccRegisterInfo;
namespace MiniRvccISD {
enum NodeType : unsigned {
    FIRST_NUMBER = ISD::BUILTIN_OP_END,
    // Return flags for different privilege levels (keep RET_FLAG for user-level)
    RET_FLAG,
    // Function call node
    CALL,
    // Conditional select and branch
    SELECT_CC,
    BR_CC,
    // Tail call
    TAIL,
    // Address calculation
    ADD_LO,
    // Get the Hi 20 bits from an address. Selected to LUI.
    HI,
    // Represents an AUIPC+ADDI pair. Selected to PseudoLLA.
    LLA,
    // Selected as PseudoAddTPRel. Used to emit a TP-relative relocation.
    ADD_TPREL,
    // Load address.
    LA_TLS_GD,
    // Multiply high for signedxunsigned.
    MULHSU,
    // READ_CYCLE_WIDE - A read of the 64-bit cycle CSR on a 32-bit target
    // (returns (Lo, Hi)). It takes a chain operand.
    READ_CYCLE_WIDE,
    // Reads value of CSR.
    // The first operand is a chain pointer. The second specifies address of the
    // required CSR. Two results are produced, the read value and the new chain
    // pointer.
    READ_CSR,
    // Write value to CSR.
    // The first operand is a chain pointer, the second specifies address of the
    // required CSR and the third is the value to write. The result is the new
    // chain pointer.
    WRITE_CSR,
    // Read and write value of CSR.
    // The first operand is a chain pointer, the second specifies address of the
    // required CSR and the third is the value to write. Two results are produced,
    // the value read before the modification and the new chain pointer.
    SWAP_CSR,
    // Load address.
    LA = ISD::FIRST_TARGET_MEMORY_OPCODE,
    LA_TLS_IE,
};
} // namespace MiniRvccISD


class MiniRvccTargetLowering : public TargetLowering {
    const MiniRvccSubtarget &Subtarget;

public:
    explicit MiniRvccTargetLowering(const TargetMachine &TM,
                                    const MiniRvccSubtarget &STI);

    const MiniRvccSubtarget &getSubtarget() const { return Subtarget; }

    bool isLegalAddressingMode(const DataLayout &DL, const AddrMode &AM, Type *Ty,
                               unsigned AS,
                               Instruction *I = nullptr) const override;
    bool isLegalICmpImmediate(int64_t Imm) const override;
    bool isLegalAddImmediate(int64_t Imm) const override;
    bool signExtendConstant(const ConstantInt *CI) const override;

    /// Return the register type for a given MVT, ensuring vectors are treated
    /// as a series of gpr sized integers.
    MVT getRegisterTypeForCallingConv(LLVMContext &Context, CallingConv::ID CC,
                                      EVT VT) const override;

    /// Return the number of registers for a given MVT, ensuring vectors are
    /// treated as a series of gpr sized integers.
    unsigned getNumRegistersForCallingConv(LLVMContext &Context,
                                           CallingConv::ID CC,
                                           EVT VT) const override;

    // Provide custom lowering hooks for some operations.
    SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
    void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue> &Results,
                            SelectionDAG &DAG) const override;

    // This method returns the name of a target specific DAG node.
    const char *getTargetNodeName(unsigned Opcode) const override;

    Instruction *emitLeadingFence(IRBuilderBase &Builder, Instruction *Inst,
                                  AtomicOrdering Ord) const override;
    Instruction *emitTrailingFence(IRBuilderBase &Builder, Instruction *Inst,
                                   AtomicOrdering Ord) const override;

    // Lower incoming arguments, copy physregs into vregs
    SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                                 bool IsVarArg,
                                 const SmallVectorImpl<ISD::InputArg> &Ins,
                                 const SDLoc &DL, SelectionDAG &DAG,
                                 SmallVectorImpl<SDValue> &InVals) const override;
    bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                        bool IsVarArg,
                        const SmallVectorImpl<ISD::OutputArg> &Outs,
                        LLVMContext &Context) const override;
    SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                        const SmallVectorImpl<ISD::OutputArg> &Outs,
                        const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
                        SelectionDAG &DAG) const override;

    SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                      SmallVectorImpl<SDValue> &InVals) const override;

private:
    SDValue lowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
};


} // end namespace llvm

#endif
