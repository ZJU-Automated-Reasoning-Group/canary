# ESSS (Error Safety and Sanity checking System)

## Overview

ESSS is a static analysis tool built on LLVM that detects error handling violations in C/C++ codebases. It focuses on identifying error propagation issues, inadequate error checks, and improper error handling patterns.

## Version

Current version: 1.0.0 (as defined in `include/Checker/ESSS/Config.h`)

## Features

- **Error Block Detection**: Identifies code blocks that handle error conditions
- **Call Graph Analysis**: Uses MLTA to Build call graphs for interprocedural analysis
- **Data Flow Analysis**: Tracks how error values propagate through functions
- **Function Error Return Intervals**: Maps functions to their error return value ranges
- **Value Set Analysis (VSA)**: Performs value set analysis on function return values

## Components

### Core Analysis Components

1. **EHBlockDetector**
   - Detects error handling blocks in functions
   - Identifies patterns of error checking and handling

2. **ErrorCheckViolationFinder**
   - Identifies violations in error check handling
   - Ensures error conditions are properly checked

3. **FunctionErrorReturnIntervals**
   - Maps functions to error return value intervals
   - Tracks which return values indicate errors

4. **Interval**
   - Represents value ranges for error checking
   - Provides interval analysis for error values

5. **DataFlowAnalysis**
   - Tracks data flow through the program
   - Follows how error values propagate

6. **CallGraph**
   - Builds and analyzes the program's call graph
   - Enables interprocedural analysis


### Helper Components

1. **Common**
   - Common utilities and functions
   - Shared type definitions

2. **Helpers**
   - Additional utility functions
   - Helper methods for analysis

3. **DebugHelpers**
   - Debugging utilities
   - Aids in troubleshooting analysis issues



## Usage

The ESSS analyzer is typically integrated into a larger analysis framework. It operates on LLVM modules and uses a context-based approach to track analysis results across modules.

