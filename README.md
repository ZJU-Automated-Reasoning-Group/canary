# Canary


Canary is a program analysis and verification framework. It provides severl toolkits that can be used
individully or in combination to perform different tasks.
The current version of Canary has been tested on x86 Linux and ARM Mac using LLVM-12 and LLVM-14 with Z3-4.11.

- DyckAA: a unification-based, exhuastive alias analysis (See `lib/DyckAA`)
- CFLAA: All files inthe subfolder are 1:1 copied from LLVM 14.0.6 and are subject to the LLVM license.
We copy these files, as LLVM removed them in the transition from version 14 to 15
- SMT Solving (See `lib/SMT`)
- Binary Decision Diagram (BDD): (See `lib/cudd`)
- kint: a static bug finder.

## Installation

Build LLVM

```bash
# Clone LLVM repository
git clone https://github.com/llvm/llvm-project.git
cd llvm-project

# Checkout desired version
git checkout llvmorg-14.0.0  # or llvmorg-12.0.0

# Build LLVM
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ../llvm
make -j$(nproc)  # Uses all available CPU cores
```

Then, you can build this projet as follow
```bash
git clone https://github.com/ZJU-Automated-Reasoning-Group/canary
cd canary
mkdir build
cd build
cmake ../ -DLLVM_BUILD_PATH=/path/to/llvm/build
make -j$(nproc)
```

Currently, we asume that the system has the right version of Z3.

TBD: download the LLVM and Z3 obj files automatically...

## Using the Alias Analysis


Build and link the lib files to your project and use `DyckAliasAnalysis` as a common Mod pass. 
You may use the following options.

* -print-alias-set-info

This will print the evaluation of alias sets and outputs all alias sets, and their 
relations (dot style).

* -count-fp

Count how many functions that a function pointer may point to.

* -no-function-type-check

If the option is set, we do not check the function FuncTy when resolving pointer
calls, otherwise, only FuncTy compatible function can be aliased with a function
pointer. We say f1 and f2 are two FuncTy-compatible functions iff.

    - Both or netheir of them are var arg function;

    - Both or netheir of them have a non-void return value;

    - Same number of parameters;

    - Same FuncTy store sizes of each pair of parameters;

    - There is an explicit cast operation between FuncTy(f1) and FuncTy(f2) (it works with option -with-function-cast-comb).

* -dot-dyck-callgraph

This option is used to print a call graph based on the alias analysis.
You can use it with -with-labels option, which will add lables (call insts)
to the edges in call graphs.

## Using Kint

## Using the SMT Solver



~~~~
./owl file.smt2
~~~~



