# MiniRvcc: Minimal LLVM Backend for RV32


This project is a minimal custom LLVM backend implementation for a 32-bit RISC-V-like target named **MiniRvcc**.

MiniRvcc is designed as a **learning-oriented backend** for compiling programs that run on a custom soft processor implemented in the [`learn_simple_soc`](https://github.com/you3kou3/learn_simple_soc) project.  
Its goal is to serve as an educational reference for how to integrate a custom target into LLVM and produce executable code for a specific SoC environment.

When built correctly, running the following command:


```bash
$ ./llc --version
LLVM (http://llvm.org/):
  LLVM version 15.0.0
  DEBUG build with assertions.
  Default target: x86_64-unknown-linux-gnu
  Host CPU: goldmont

  Registered Targets:
    minirvcc - 32-bit Mini RV32
```

## LLVM Version
This project is based on LLVM 15.0.0.

## Step 1: Register minirvcc Target in LLVM
To integrate MiniRvcc into LLVM, several core files must be modified:

llvm/CMakeLists.txt

llvm/cmake/config-ix.cmake

llvm/ADT/Triple.h

llvm/BinaryFormat/ELF.h

llvm/lib/Support/Triple.cpp

These changes add support for the minirvcc target triple and allow it to be recognized by LLVM tooling.


## Step 2: Copy MiniRvcc Source
Copy this GitHub repository folder (i.e., the MiniRvcc backend source code) into the following path inside the LLVM source tree:

```bash
llvm-project/llvm/lib/Target/MiniRvcc
```


## Step 3: Configure the Build
Create a separate build directory for MiniRvcc:

```bash
mkdir llvm-project/build_minirvcc
cd llvm-project/build_minirvcc
```

Run cmake to configure LLVM with only the MiniRvcc target:

```bash
cmake ../llvm -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_TARGETS_TO_BUILD="MiniRvcc"
```

Note:

Replace ../llvm with the actual path to your cloned LLVM source directory if different.

This limits the build to just the MiniRvcc target to reduce build time.


## Step 4: Build with Ninja
Execute the build:

```bash
ninja
```

If successful, the llc binary will be generated at:

```bash
llvm-project/build_minirvcc/bin/llc
```

## Step 5: Verify MiniRvcc is Registered
Run:

```bash
./bin/llc --version
```

You should see the following line among the registered targets:

```bash
minirvcc - 32-bit Mini RV32
```

This confirms that the MiniRvcc backend has been successfully integrated into your LLVM build.

## Notes

This backend currently only registers the target with LLVM and does not yet generate real code.

Future work may include implementing instruction selection, code emission, and ABI support.

