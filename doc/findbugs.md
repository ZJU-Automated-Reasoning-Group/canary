# Bug Finding Tools for C/C++ Programs


This document provides an overview of bug finding tools for C/C++ programs. 

## Tool Categories

Bug finding tools for C/C++ can be categorized based on their underlying technology and approach:


| Tool | Description | Link |
|------|-------------|------|
| Infer | A static analysis tool developed by Facebook, capable of identifying null pointer dereferences, memory leaks, and resource leaks in C, C++, and other languages. | [GitHub](https://github.com/facebook/infer) |
| CPPCheck | A static analysis tool that detects bugs, undefined behaviors, and dangerous coding patterns in C/C++ code. | [Website](http://cppcheck.sourceforge.net/) |
| Flawfinder | A simple static analyzer that scans C/C++ code for potential security vulnerabilities. | [GitHub](https://github.com/david-a-wheeler/flawfinder) |
| Semgrep | A lightweight static analysis tool for many languages including C/C++. It uses pattern matching to find bugs and enforce code standards. | [Website](https://semgrep.dev/) |
| CodeQL | A semantic code analysis engine developed by GitHub that treats code as data to find security vulnerabilities and quality issues in C/C++ programs. | [Website](https://codeql.github.com/) |

### Clang-based Tools

Clang-based tools leverage the Clang compiler infrastructure to perform various analyses.

| Tool | Description | Link |
|------|-------------|------|
| Clang Static Analyzer | A source code analysis tool that finds bugs in C, C++, and Objective-C programs, part of the LLVM/Clang project. | [Website](https://clang-analyzer.llvm.org/) |
| Clang-Tidy | A clang-based C++ linter tool with an extensible framework for diagnosing and fixing typical programming errors. | [Documentation](https://clang.llvm.org/extra/clang-tidy/) |
| Ericsson CodeChecker | A static analysis infrastructure built on the LLVM/Clang Static Analyzer. | [GitHub](https://github.com/Ericsson/codechecker) |
| Naive Systems Analyzer | A static code analyzer built on Clang for finding bugs in C/C++ code. | [GitHub](https://github.com/naivesystems/analyze) |
| Cooddy | An SAST (Static Application Security Testing) tool for C/C++ code. | [GitHub](https://github.com/program-analysis-team/cooddy) |

### LLVM IR-based Tools

These tools perform analysis at the LLVM Intermediate Representation (IR) level.

| Tool | Description | Link |
|------|-------------|------|
| SVF | A static tool that enables scalable and precise interprocedural dependence analysis for C and C++ programs. | [GitHub](https://github.com/SVF-tools/SVF) |
| Canary | A C/C++ analysis tool developed by the ZJU Automated Reasoning Group. | [GitHub](https://github.com/ZJU-Automated-Reasoning-Group/canary) |
| CLAM | A static analysis framework for LLVM IR based on abstract interpretation. | [GitHub](https://github.com/seahorn/clam) |
| IKOS | A static analyzer for C/C++ based on the theory of Abstract Interpretation, developed by NASA. | [GitHub](https://github.com/NASA-SW-VnV/ikos) |
| SMACK | A modular software verification toolchain and bug finder. | [GitHub](https://github.com/smackers/smack) |
| SeaHorn | A software model checker for LLVM-based languages. | [GitHub](https://github.com/seahorn/seahorn) |




## Additional Resources

- [LLVM](https://llvm.org/) - The foundation for many of these tools
- [Common Weakness Enumeration (CWE)](https://cwe.mitre.org/) - A community-developed list of common software security weaknesses
- [OWASP C-based Toolchain Hardening Guide](https://cheatsheetseries.owasp.org/cheatsheets/C-Based_Toolchain_Hardening_Cheat_Sheet.html) - Best practices for hardening C/C++ code


## Kint Bug Detector Checkers

Kint is a static analyzer for detecting integer bugs in C/C++ programs. It uses LLVM IR as its input and can detect various types of integer-related bugs.

### Available Checkers

The following checkers are available in Kint:

1. **Integer Overflow** (`check-int-overflow`): Detects integer overflow conditions in arithmetic operations (addition, subtraction, multiplication, division).

2. **Division by Zero** (`check-div-by-zero`): Detects potential division by zero conditions.

3. **Bad Shift** (`check-bad-shift`): Detects shift operations with excessive shift amounts (e.g., shifting a 32-bit value by 32 or more bits).

4. **Array Out of Bounds** (`check-array-oob`): Detects array index out of bounds conditions when accessing arrays.

5. **Dead Branch** (`check-dead-branch`): Detects branches that cannot be taken due to contradictory conditions.

Since all checkers are disabled by default, you only need to enable the specific bug types you're interested in. This allows for targeted analysis and better performance when analyzing large codebases. 

### Usage

By default, all checkers are disabled. You can enable checkers using command-line flags:

```bash
# Enable all checkers at once
./kint input.ll --check-all=true

# Enable specific checkers
./kint input.ll --check-int-overflow=true --check-div-by-zero=true

# Enable only array out of bounds checking
./kint input.ll --check-array-oob=true

# Enable all except dead branch detection
./kint input.ll --check-all=true --check-dead-branch=false
```

### Logging Options

Kint provides several options to control the logging output:

You can set the verbosity of the log output using the `--log-level` option:

```bash
# Show all messages including debug information
./kint input.ll --log-level=debug

# Show informational messages and above (default)
./kint input.ll --log-level=info

# Show only warnings and errors
./kint input.ll --log-level=warning

# Show only errors
./kint input.ll --log-level=error

# Suppress all log output
./kint input.ll --log-level=none
```

You can control where log messages are sent:

```bash
# Send logs to stderr instead of stdout
./kint input.ll --log-to-stderr

# Send logs to a file
./kint input.ll --log-to-file=kint.log

# Suppress most log output (equivalent to --log-level=none)
./kint input.ll --quiet
```


