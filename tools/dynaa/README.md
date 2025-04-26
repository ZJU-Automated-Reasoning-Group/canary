# Dynamic Alias Analysis Checker


(upgraded to LLVM 14 from https://github.com/rainoftime/neonGobyPort)

This tool dynamically observes pointer addresses in a test program and checks these addresses against static alias analysis results to find potential alias analysis errors.

## Prerequisites

- LLVM 14.0.0
- Boost libraries

## Offline Mode

To check an alias analysis (say `basic-aa`) with a test program (say `example.cpp`) using the offline mode, follow these steps:

1. Compile the code into LLVM's intermediate representation (IR):
```bash
clang -emit-llvm -c example.cpp -o example.bc
```

2. Instrument the code for dynamic analysis:
```bash
./bin/dynaa-instrument example.bc -o example.inst.bc
```

3. Compile the instrumented IR and link with the runtime library:
```bash
clang example.inst.bc libRuntime.a -o example.inst
```

4. Run the instrumented program to collect runtime pointer information:
```bash
LOG_DIR=<log-dir> ./example.inst
```
This will generate a log file at `<log-dir>/pts.log`. You can specify any directory using the `LOG_DIR` environment variable. If not specified, the log will be written to the current directory.

5. Check the collected information against a static alias analysis:
```bash
./bin/dynaa-check example.bc <log-dir>/pts.log basic-aa
```
This will report any inconsistencies between the dynamic observations and the static alias analysis results.

## Dumping Logs

Use `dynaa-log-dump` to dump the binary `pts.log` files to a readable format:

```bash
./bin/dynaa-log-dump <log-dir>/pts.log
```

## Notes

- This is a modified version of the [original NeonGoby project](https://github.com/wujingyue/neongoby), updated to work with LLVM 14.
- Support for additional alias analyses can be added by extending `dynaa-check.cpp`.
