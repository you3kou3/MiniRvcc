//===-- MiniRvccRegisterInfo.td - MiniRvcc Register defs ---*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the MiniRvcc register file
//===----------------------------------------------------------------------===//


let Namespace = "MiniRvcc" in {
  class MiniRvccReg<bits<5> Enc, string n, list<string> alt = []> : Register<n> {
    let HWEncoding{4-0} = Enc;
    let AltNames = alt;
  }
}


def ABIRegAltName : RegAltNameIndex;
let RegAltNameIndices = [ABIRegAltName] in {
  def X0  : MiniRvccReg<0,  "x0", ["zero"]>,  DwarfRegNum<[0]>;
  let CostPerUse = [0, 1] in {
  def X1   : MiniRvccReg<1,  "x1", ["ra"]>,    DwarfRegNum<[1]>;
  def X2   : MiniRvccReg<2,  "x2", ["sp"]>,    DwarfRegNum<[2]>;
  def X3   : MiniRvccReg<3,  "x3", ["gp"]>,    DwarfRegNum<[3]>;
  def X4   : MiniRvccReg<4,  "x4", ["tp"]>,    DwarfRegNum<[4]>;
  def X5   : MiniRvccReg<5,  "x5", ["t0"]>,    DwarfRegNum<[5]>;
  def X6   : MiniRvccReg<6,  "x6", ["t1"]>,    DwarfRegNum<[6]>;
  def X7   : MiniRvccReg<7,  "x7", ["t2"]>,    DwarfRegNum<[7]>;
  }
  def X8   : MiniRvccReg<8,  "x8", ["s0", "fp"]>, DwarfRegNum<[8]>;
  def X9   : MiniRvccReg<9,  "x9", ["s1"]>,    DwarfRegNum<[9]>;
  def X10  : MiniRvccReg<10, "x10", ["a0"]>,   DwarfRegNum<[10]>;
  def X11  : MiniRvccReg<11, "x11", ["a1"]>,   DwarfRegNum<[11]>;
  def X12  : MiniRvccReg<12, "x12", ["a2"]>,   DwarfRegNum<[12]>;
  def X13  : MiniRvccReg<13, "x13", ["a3"]>,   DwarfRegNum<[13]>;
  def X14  : MiniRvccReg<14, "x14", ["a4"]>,   DwarfRegNum<[14]>;
  def X15  : MiniRvccReg<15, "x15", ["a5"]>,   DwarfRegNum<[15]>;
  let CostPerUse = [0, 1] in {
  def X16  : MiniRvccReg<16, "x16", ["a6"]>,   DwarfRegNum<[16]>;
  def X17  : MiniRvccReg<17, "x17", ["a7"]>,   DwarfRegNum<[17]>;
  def X18  : MiniRvccReg<18, "x18", ["s2"]>,   DwarfRegNum<[18]>;
  def X19  : MiniRvccReg<19, "x19", ["s3"]>,   DwarfRegNum<[19]>;
  def X20  : MiniRvccReg<20, "x20", ["s4"]>,   DwarfRegNum<[20]>;
  def X21  : MiniRvccReg<21, "x21", ["s5"]>,   DwarfRegNum<[21]>;
  def X22  : MiniRvccReg<22, "x22", ["s6"]>,   DwarfRegNum<[22]>;
  def X23  : MiniRvccReg<23, "x23", ["s7"]>,   DwarfRegNum<[23]>;
  def X24  : MiniRvccReg<24, "x24", ["s8"]>,   DwarfRegNum<[24]>;
  def X25  : MiniRvccReg<25, "x25", ["s9"]>,   DwarfRegNum<[25]>;
  def X26  : MiniRvccReg<26, "x26", ["s10"]>,   DwarfRegNum<[26]>;
  def X27  : MiniRvccReg<27, "x27", ["s11"]>,   DwarfRegNum<[27]>;
  def X28  : MiniRvccReg<28, "x28", ["t3"]>,   DwarfRegNum<[28]>;
  def X29  : MiniRvccReg<29, "x29", ["t4"]>,   DwarfRegNum<[29]>;
  def X30  : MiniRvccReg<30, "x30", ["t5"]>,   DwarfRegNum<[30]>;
  def X31  : MiniRvccReg<31, "x31", ["t6"]>,   DwarfRegNum<[31]>;
  }
}


def XLenVT : ValueType<32, i32>;

def GPR : RegisterClass<"MiniRvcc", [XLenVT], 32, (add
    (sequence "X%u", 10, 17),   //a0, a1, a2, a3, a4, a5, a6, a7
    (sequence "X%u", 5, 7),     //t0, t1, t2
    (sequence "X%u", 28, 31),   //t3, t4, t5, t6
    (sequence "X%u", 8, 9),     //s0(fp), s1
    (sequence "X%u", 18, 27),   //s2, s3, s4, s5, s6, s7, s8, s9, s10, s11
    (sequence "X%u", 0, 4)      //zero, ra, sp, gp, tp
  )> {
  let RegInfos = XLenRI;
}

