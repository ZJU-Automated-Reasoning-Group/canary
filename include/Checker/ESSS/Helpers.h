#ifndef ESSS_CHECKER_HELPERS_H
#define ESSS_CHECKER_HELPERS_H

#pragma once

#include <string>
#include <vector>
#include <set>
#include <unordered_set>
#include <span>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/ADT/Optional.h>
#include "Analyzer.h"
#include "Lazy.h"


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
llvm::Optional<CalleeMap::const_iterator> getCalleeIteratorForPotentialCallInstruction(const GlobalContext& Ctx, const Instruction& instruction);
llvm::Optional<vector<string>> getListOfTestCases();
float wilsonScore(float positive, float n, float z);
bool isProbablyPure(const Function* function);

#endif // ESSS_CHECKER_HELPERS_H
