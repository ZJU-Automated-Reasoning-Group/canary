#pragma once

#include "Analyzer.h"
#include "Common.h"
#include "PathSpan.h"


enum class OperationType : unsigned char {
    Call,
    Return,
    Unreachable,
    CondBr,
    Switch,
    Store,
};

struct Path {
    vector<const BasicBlock*> blocks;
    const AbstractCondition* reason;
};

struct Operation {
    OperationType type;
    union {
        ICmpInst::Predicate predicate; // Here for struct packing
    };
    union {
        const FuncSet *callTargets;
        const Value *value;
        struct {
            const Value* resolvedValue, *unresolvedValue;
        } returnData;
        struct {
            const Value* value;
            const Instruction* instruction;
        } condBrData;
        struct {
            const Value* value;
            const Instruction* instruction;
        } storeData;
    };

    bool resolvePathSensitiveValues(const vector<const BasicBlock*>& blocks);

    bool operator==(const Operation& other) const;
};

struct Summary {
    vector<Operation> ops;
    const BasicBlock* originalBlockIndex1;

    bool resolvePathSensitiveValues(const vector<const BasicBlock*>& blocks);

    void merge(const Summary& summary) {
        ops.reserve(summary.ops.size());
        ops.insert(ops.end(), summary.ops.begin(), summary.ops.end());
    }

    unsigned int numberOfCondBrs() const;

    unsigned int numberOfSameCondBrs(const Summary& other) const;

    void dump() const;
};

struct SafetyCheckData {
    unsigned short lcs {};
    unsigned short pathLength {numeric_limits<unsigned short>::max()};
    float ratio {};
    const BasicBlock* errorHandlingBlock {};
    unsigned short sumOfCondBrCount {numeric_limits<unsigned short>::max()};
};

class EHBlockDetectorPass : public IterativeModulePass {

public:

    explicit EHBlockDetectorPass(GlobalContext *Ctx_)
            : IterativeModulePass(Ctx_, "EHBlockDetector") {
    }
    bool doInitialization(llvm::Module *) override;
    bool doFinalization(llvm::Module *) override;
    void doModulePass(llvm::Module *) override;
    void associationAnalysisForErrorHandlers();
    void storeData();
    inline void nextStage() { stage++; }
    void propagateCheckedErrors();
    void learnErrorsFromErrorBlocksForSelf();

    static void collectPaths(const BasicBlock* currentBlock, vector<Path*>& allPaths, Path* myCurrentPath, set<const BasicBlock*>& basicBlocksOfNonInterest);
    static llvm::Optional<bool> determineErrorBranchOfCallWithCompare(ICmpInst::Predicate predicate, unsigned int returnValueIndex, int rhs, const CallInst* checkedCall);

private:
    void stage0(llvm::Module *);
    void stage1(llvm::Module *);
    void processSafetyCheckMapping(const map<const AbstractComparison*, SafetyCheckData>& mapping);

    Summary summarizeBlock(const BasicBlock* currentBlock) const;
    void identifyPotentialSanityChecks(const Function& function);
    static void collectPathsAux(const BasicBlock* currentBlock, vector<Path*>& allPaths, Path* myCurrentPath, set<const BasicBlock*>& visited, set<const BasicBlock*>& basicBlocksOfNonInterest, const BasicBlock* lastBr);
    const BasicBlock* determineSuccessorOfAbstractComparisonWhichHandlesErrors(const AbstractComparison* abstractComparison) const;
    const BasicBlock* determineSuccessorOfAbstractComparisonWhichHandlesErrors(const BasicBlock* abstractComparisonBlock) const;
    llvm::Optional<Interval> addForSpanAndReturnInstruction(PathSpan pathSpan, const ReturnInst* returnInstruction);

    map<const Function*, InErrorNotInErrorPair> functionToInErrorNotInErrorPair;
    FunctionToIntervalCounts functionToIntervalCounts;
    map<const Module*, map<const AbstractComparison*, SafetyCheckData>> moduleToSafetyChecks;
    map<const AbstractComparison*, pair<const Value*, unsigned int>> conditionalToAction;
    set<const Function*> associatedErrorHandlerFunctions;
    int stage = 0;
};
