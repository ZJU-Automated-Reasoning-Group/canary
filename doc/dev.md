# Development Guide

## TODO LIST


### Testing

- Add unit test (at least for the SMT library)


### New Features
- Integrate the pre-condition inference engine in Beacon (SP'22), which relies on some code from KLEE (it does not use the symbolic execution engine in KLEE) and SVF. 
- Integrate SVF as a library, which implementes several pointer analsyes with different precisions.
- IR optimization: reduant load/store elimination, control-flow refinement, superoptimization, etc.
- Slicing (e.g., conventional slicing, thin slicing, hybrid thin slicing, ...)
- Instrumentation (e.g., to guide fuzzing, to profile, etc.)
- (Low priority) Symbolic emulation/execution (e.g., KLEE, ...)
- (Low priority) Numerical abstract interpretation (e.g., IKOS, CLAM/Crab, ...)
- (Low priority) Software model checking (e.g., Smarck, Seahorn, ...)
- (Low priority) Analyzing IR lifted from binaries (e.g., by Remill, retdec, ...)
- (Low priority) Clang AST-based bug checking (e.g., Clang static analyzer, ...)
- (Low priority) Data flow analsyis frameowrk (e.g., Pharsar, ...)