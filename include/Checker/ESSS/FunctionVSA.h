#pragma once

#include <llvm/IR/ConstantRange.h>
#include <set>
#include <map>
#include "Checker/ESSS/Analyzer.h"

class FunctionErrorReturnIntervals;

using namespace std;
using namespace llvm;

class FunctionVSA {
private:
    using ValueSet = SmallPtrSet<const Value*, 4>;
public:
    explicit FunctionVSA(const FunctionToIntervalCounts& allIntervals) : allIntervals(allIntervals) {}

    IntervalHashMap refine(const Function* function, IntervalHashMap& map);
    ConstantRange computeConstantRangeFor(const Function* function, ValueSet& valueSet);
    ConstantRange computeConstantRangeFor(const Value* V, const Instruction* C, ValueSet& valueSet);

    map<const Function*, ConstantRange> functionToRange;
    const FunctionToIntervalCounts& allIntervals;
};
