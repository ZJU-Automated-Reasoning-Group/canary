#pragma once

#include <string>
#include <optional>
#include <vector>
#include <set>
#include <unordered_set>
#include <span>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Analysis/PostDominators.h>
#include "Checker/ESSS/Analyzer.h"
#include "Checker/ESSS/Lazy.h"


struct GlobalContext;


using namespace llvm;
using namespace std;


template<typename T>
class Deferred {
public:
    explicit Deferred(T destructor) : destructor(destructor) {}
    ~Deferred() { destructor(); }

private:
    T destructor;
};

bool isCompositeType(const Type* type);
bool canBeUsedInAnIndirectCall(const Function& function);
optional<CalleeMap::const_iterator> getCalleeIteratorForPotentialCallInstruction(const GlobalContext& Ctx, const Instruction& instruction);
optional<vector<string>> getListOfTestCases();
float wilsonScore(float positive, float n, float z);
bool isProbablyPure(const Function* function);
