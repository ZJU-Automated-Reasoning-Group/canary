#ifndef ANALYZER_GLOBAL_H
#define ANALYZER_GLOBAL_H

#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Support/CommandLine.h>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "Checker/ESSS/Common.h"
#include "Checker/ESSS/FunctionErrorReturnIntervals.h"


using CallInstSetEntry = llvm::CallInst*;

// 
// typedefs
//
using ModuleMap = DenseMap<Module*, StringRef>;
// The set of all functions.
typedef llvm::SmallPtrSet<llvm::Function*, 2> FuncSet;
// Mapping from function name to function.
typedef unordered_map<string_view, llvm::Function*> NameFuncMap;
typedef llvm::SmallPtrSet<CallInstSetEntry, 2> CallInstSet;
typedef DenseMap<Function*, CallInstSet> CallerMap;
typedef DenseMap<CallInst *, FuncSet> CalleeMap;
// Pointer analysis types.
typedef DenseMap<Value *, SmallPtrSet<Value *, 16>> PointerAnalysisMap;
typedef unordered_map<Function *, PointerAnalysisMap> FuncPointerAnalysisMap;

struct InErrorNotInErrorPair {
    unsigned int inError {}, notInError {};
};

using IntervalHashMap = unordered_map<Interval, unsigned int, IntervalHash>;
using FunctionToIntervalCounts = DenseMap<pair<const Function*, unsigned int>, IntervalHashMap>;

struct GlobalContext {
	// Map global function name to function.
	NameFuncMap GlobalFuncs;

	// Map a callsite to all potential callee functions.
	CalleeMap Callees;

	// Map a function to all potential caller instructions.
	CallerMap Callers;

	// Unified functions -- no redundant inline functions
	DenseMap<size_t, Function *>UnifiedFuncMap;
	DenseSet<const Function *>UnifiedFuncSet;

	// Map function signature to functions
	DenseMap<size_t, FuncSet>sigFuncsMap;

	// Modules.
	ModuleMap Modules;

	// Pointer analysis results.
    FuncPointerAnalysisMap FuncPAResults;

    DenseSet<const CallInst*> Layer1OnlyCalls;

    map<const Module*, AAResultsWrapperPass*> AAPass;

    // Error handling rules
    struct {
        map<const Function*, vector<pair<pair<const Value*, unsigned int>, const class AbstractCondition*>>> functionToSanityValuesAndConditions;
        FunctionErrorReturnIntervals functionErrorReturnIntervals;
        mutex functionToConfidenceMutex;
        map<pair<const Function*, unsigned int>, float> functionToConfidence;
    };

	bool shouldSkipFunction(const Function* function) const {
		return function->empty() || UnifiedFuncSet.find(function) == UnifiedFuncSet.end();
	}
};

// Forwards
class AbstractComparison;

class IterativeModulePass {
protected:
	GlobalContext *Ctx;
	const char * ID;
public:
	IterativeModulePass(GlobalContext *Ctx_, const char *ID_)
		: Ctx(Ctx_), ID(ID_) { }

	// Run on each module before iterative pass.
	virtual bool doInitialization(llvm::Module *M)
		{ return true; }

	// Run on each module after iterative pass.
	virtual bool doFinalization(llvm::Module *M)
		{ return true; }

	// Iterative pass.
	virtual void doModulePass(llvm::Module *M)
		{  }

	virtual void run(ModuleMap &modules, bool multithreaded=false);

private:
    void _doModulePass(llvm::Module* M) {
        //OP << M->getName() << "\n";
        doModulePass(M);
    }
};

#endif
