# Dataflow Analysis Framework

This directory contains a generic framework for dataflow analysis and implementations of common dataflow analyses including:

1. Liveness Analysis
2. Reaching Definitions Analysis

## Overview

The dataflow analysis framework is designed to be extensible, allowing users to implement new analyses easily by inheriting from the base `DataflowAnalysis` class and implementing the required transfer functions.

## Usage

### As a Pass

Analyses can be used as either a new-style analysis pass or a legacy pass. For example, with `opt`:

```bash
# Using legacy pass with opt
opt -load /path/to/libDataflowAnalysis.so -liveness file.ll -analyze

# Using legacy pass with reaching definitions
opt -load /path/to/libDataflowAnalysis.so -reaching-defs file.ll -analyze
```

### Programmatic API

```cpp
#include "Dataflow/LivenessAnalysis.h"
#include "Dataflow/ReachingDefinitions.h"

using namespace canary;

// For LivenessAnalysis:
LivenessAnalysis LVA(MyFunction);
LVA.run();

// For a specific basic block, get live variables:
const BasicBlock *BB = ...;
const LivenessDomain &LiveVars = LVA.getResult(BB);

// Check if a variable is live at the entry of a block:
if (LiveVars.isLive(MyValue)) {
  // Value is live
}

// For ReachingDefsAnalysis:
ReachingDefsAnalysis RDA(MyFunction);
RDA.run();

// For a specific instruction, get reaching definitions:
const Instruction *I = ...;
const ReachingDefsDomain &ReachingDefs = RDA.getReachingDefs(I);

// Get all definitions that reach a specific value:
const Value *V = ...;
const ReachingDefsDomain::InstSetType &Defs = ReachingDefs.getDefinitions(V);
for (const Instruction *DefInst : Defs) {
  // Process each definition
}
```

## Extending the Framework

To create a new dataflow analysis:

1. Create a new domain class that implements the required operations:
   - `meet()` method for combining dataflow values
   - `equals()` method for comparing dataflow values

2. Implement a new analysis class inheriting from `DataflowAnalysis`:
   - Implement `transferInstruction()` for the transfer function
   - Implement `initialState()` for the initial dataflow values

3. Optionally, implement pass manager integration by creating a pass class

Example:

```cpp
// A simple constant propagation domain
class ConstPropDomain {
public:
  // Domain implementation...
  ConstPropDomain meet(const ConstPropDomain &Other) const;
  bool equals(const ConstPropDomain &Other) const;
};

// The analysis
class ConstantPropagation : public DataflowAnalysis<ConstPropDomain, Direction::Forward> {
public:
  explicit ConstantPropagation(Function &F) : DataflowAnalysis(F) {}
  void initialize() override;
  ConstPropDomain transferInstruction(const Instruction &I, const ConstPropDomain &State) override;
  ConstPropDomain initialState() override;
};
```

## TODOs

1. Implement more classic dataflow analyses:
   - Available Expressions
   - Very Busy Expressions
   - Copy Propagation

2. Add interprocedural support
3. Add sparse dataflow optimizations
4. Add multithreading support for large functions 