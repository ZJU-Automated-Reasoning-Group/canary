# Canary


Canary is a program analysis and verification framework. It provides severaal toolkits that can be used
individully or in combination to perform different tasks.
The current version of Canary has been tested on x86 Linux architectures using LLVM-12 and Z3-4.11.

- Alias Analysis: a unification-based, exhuastive alias analysis (See `lib/DyckAA`)
- SMT Solving (See `lib/SMT`)
- Binary Decision Diagram (BDD): (See `lib/cudd`)


## Building 


```bash
git clone https://github.com/ZJU-Automated-Reasoning-Group/canary
cd canary
mkdir build
cd build
cmake ..
make
```

Currently, we asume that the system has the right versions of LLVM and Z3.


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


## Using the SMT Solver


~~~~
./owl file.smt2
~~~~

## TODO LIST



- Add unit test (at least for the SMT library)
- Integrate the pre-condition inference engine in Beacon (SP'22), which relies on some code from KLEE (it does not use the symbolic execution engine in KLEE) and SVF. 
- Integrate SVF as a library, which implementes several pointer analsyes with different precisions.
- IR optimization: reduant load/store elimination, control-flow refinement, superoptimization, etc.
- Slicing (e.g., conventional slicing, thin slicing, hybrid thin slicing, ...)
- Instrumentation
- (Low priority) Symbolic emulation/execution (e.g., KLEE, ...)
- (Low priority) Numerical abstract interpretation (e.g., IKOS, CLAM/Crab, ...)
- (Low priority) Software model checking (e.g., Smarck, Seahorn, ...)
- (Low priority) Analyzing IR lifted from binaries (e.g., by Remill, retdec, ...)
- (Low priority) Clang AST-based bug checking (e.g., Clang static analyzer, ...)
- (Low priority) Data flow analsyis frameowrk (e.g., Pharsar, ...)

