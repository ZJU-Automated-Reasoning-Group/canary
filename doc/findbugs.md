# Bug Finding Tools for C/C++ Programs

## Introduction

This document provides an overview of bug finding tools for C/C++ programs. 

## Tool Categories

Bug finding tools for C/C++ can be categorized based on their underlying technology and approach:

### Other Tools

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

