# Lotus

Lotus is a program analysis, verification, and optimization framework. It provides several toolkits that can be used
individually or in combination to perform different tasks.
The current version of Canary has been tested on x86 Linux and ARM Mac using LLVM-12 and LLVM-14 with Z3-4.11.

## Major Components

### Alias Analysis

- **DyckAA**: A unification-based, exhaustive alias analysis (See `lib/DyckAA`)
- **CFLAA**: All files in the subfolder are 1:1 copied from LLVM 14.0.6 and are subject to the LLVM license.
  We copy these files as LLVM removed them in the transition from version 14 to 15
- **Sea-DSA**: A context-sensitive, field-sensitive alias analysis based on Data Structure Analysis (See `lib/seadsa`)
- **Andersen**: Context-insensitive points-to analysis implementation (without on-the-fly callgraph construction)

### Constraint Solving

- **SMT Solving**: Z3 integration (See `lib/Solvers/SMT`)
- **Binary Decision Diagram (BDD)**: CUDD-based implementation (See `lib/Solvers/CUDD`)
- **WPDS**: Weighted Pushdown System library for program analysis (See `lib/Solvers/WPDS`)

### Bug Finding

- **Kint**: A static bug finder for integer-related vulnerabilities?
- **ESSS**: For finding error checking bugs?

### Utilities

- **NullPointer**: Null pointer analysis.
- **RapidJSON**: A fast JSON parser/generator for C++ with both SAX/DOM style API (See `include/rapidjson`)
- **Transform**: Tranformations for BC

## Installation

### Prerequisites

- LLVM 12.0.0 or 14.0.0
- Z3 4.11
- CMake 3.10+
- C++14 compatible compiler

### Build LLVM

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

### Build Canary

```bash
git clone https://github.com/ZJU-Automated-Reasoning-Group/lotus
cd lotus
mkdir build && cd build
cmake ../ -DLLVM_BUILD_PATH=/path/to/llvm/build
make -j$(nproc)
```

**Note**: The build system currently assumes that the system has the correct version of Z3 installed.

The build system will automatically download and build Boost if it's not found on your system. You can specify a custom Boost installation path with `-DCUSTOM_BOOST_ROOT=/path/to/boost`.

> **TODO**: Implement automatic download of LLVM and Z3 dependencies

## Usage Guides

### Using DyckAA

Build and link the library files to your project and use `DyckAliasAnalysis` as a common LLVM ModulePass. 
Available options:

* `-print-alias-set-info`
  
  Prints the evaluation of alias sets and outputs all alias sets and their relations (DOT format).

* `-count-fp`
  
  Counts how many functions a function pointer may point to.

* `-no-function-type-check`
  
  If set, disables function type checking when resolving pointer calls. Otherwise, only FuncTy-compatible functions can be aliased with a function pointer. Two functions f1 and f2 are FuncTy-compatible if:
  
  - Both or neither are variadic functions
  - Both or neither have a non-void return value
  - They have the same number of parameters
  - Parameters have the same FuncTy store sizes
  - There is an explicit cast operation between FuncTy(f1) and FuncTy(f2) (works with `-with-function-cast-comb` option)

* `-dot-dyck-callgraph`
  
  Prints a call graph based on the alias analysis. Can be used with `-with-labels` option to add labels (call instructions) to the edges in call graphs.

### Using Sea-DSA

Sea-DSA is a context-sensitive, field-sensitive pointer analysis based on DSA (Data Structure Analysis) for analyzing memory graphs and detecting memory-related issues.


**seadsa-dg**

A simple tool for generating memory graphs:

```bash
seadsa-dg [options] <input LLVM bitcode file>
```

Options:
- `--sea-dsa-dot`: Generate DOT files visualizing the memory graphs

**seadsa-tool**

```bash
seadsa-tool [options] <input LLVM bitcode file>
```

Key options:
- `--sea-dsa-dot`: Generate DOT files visualizing memory graphs
- `--sea-dsa-callgraph-dot`: Generate DOT files of the complete call graph (currently disabled in this version)
- `--outdir <DIR>`: Specify an output directory for generated files

### Using Kint

(Documentation to be added)

### Using the SMT Solver

```bash
./owl file.smt2
```

## Related Work

- [DG](https://github.com/mchalupa/dg) - Dependence Graph for analysis of LLVM bitcode
- [AserPTA](https://github.com/PeimingLiu/AserPTA) - Andersen's points-to analysis
- [TPA](https://github.com/grievejia/tpa) - A flow-sensitive, context-sensitive pointer analysis
- [Andersen](https://github.com/grievejia/andersen) - Andersen's points-to analysis
- [SUTURE](https://github.com/seclab-ucr/SUTURE) - Static analysis for security
- [LLVM Opt Benchmark](https://github.com/dtcxzyw/llvm-opt-benchmark) - LLVM optimization benchmarks
- [EOS](https://github.com/gpoesia/eos) - ?
- https://github.com/hotpeperoncino/sfs