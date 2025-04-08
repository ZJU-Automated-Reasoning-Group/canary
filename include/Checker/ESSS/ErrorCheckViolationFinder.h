#pragma once

#include "Checker/ESSS/Analyzer.h"
#include "Common.h"

class ErrorCheckViolationFinderPass : public IterativeModulePass {
public:
    explicit ErrorCheckViolationFinderPass(GlobalContext *Ctx_)
            : IterativeModulePass(Ctx_, "ErrorCheckViolationFinder") {
    }
    bool doInitialization(llvm::Module *) override;
    bool doFinalization(llvm::Module *) override;
    void doModulePass(llvm::Module *) override;
    void finish();
    inline void nextStage() { stage++; }
    void stage0(Module*);
    void stage1(Module*);

    void determineMissingChecksAndPropagationRules(const Function& function, const FunctionErrorReturnIntervals& inputErrorIntervals, FunctionErrorReturnIntervals& outputErrorIntervals, set<const Function*>& functionsToInspectNext, unordered_set<uintptr_t>& handledFunctionPairs, map<pair<const Function*, unsigned int>, Interval>& replaceMap);
    void determineIncorrectChecks(const Function& function);
    void determineTruncationBugs() const;
    void determineSignednessBugs() const;
    void performReplaces(map<pair<const Function*, unsigned int>, Interval>& replaceMap);
    void report() const;

private:
    struct IncorrectCheckErrorReport {
        optional<pair<Interval, Interval>> intervals;
        const CallInst* call;
    };

    struct CountPair {
        float incorrect = 0, total = 0;

        CountPair() : incorrect(0), total(0) {}
        CountPair(float incorrect, float total) : incorrect(incorrect), total(total) {}

        CountPair operator+(const CountPair& other) const {
            return {incorrect + other.incorrect, total + other.total};
        }
    };

    unordered_map<SourceLocation, vector<IncorrectCheckErrorReport>, SourceLocationHasher> incorrectErrorReports;
    DenseSet<const void*> visited;

    enum class CountPairType {
        Missing, Incorrect,
    };

#ifdef SPLIT_MISSING_AND_INCORRECT_COUNTS
    DenseMap<const Function*, CountPair> errorFunctionToCountPairsIncorrect, errorFunctionToCountPairsMissing;

    DenseMap<const Function*, CountPair>& errorFunctionToCountPairsFor(CountPairType countPairType) {
        if (countPairType == CountPairType::Incorrect) {
            return errorFunctionToCountPairsIncorrect;
        } else {
            return errorFunctionToCountPairsMissing;
        }
    }

    const DenseMap<const Function*, CountPair>& errorFunctionToCountPairsFor(CountPairType countPairType) const {
        if (countPairType == CountPairType::Incorrect) {
            return errorFunctionToCountPairsIncorrect;
        } else {
            return errorFunctionToCountPairsMissing;
        }
    }
#else
    DenseMap<const Function*, CountPair> errorFunctionToCountPairs;

    DenseMap<const Function*, CountPair>& errorFunctionToCountPairsFor(CountPairType countPairType) {
        (void) countPairType;
        return errorFunctionToCountPairs;
    }

    const DenseMap<const Function*, CountPair>& errorFunctionToCountPairsFor(CountPairType countPairType) const {
        (void) countPairType;
        return errorFunctionToCountPairs;
    }
#endif

    int stage = 0;
};
