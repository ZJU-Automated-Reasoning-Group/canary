#ifndef DATA_FLOW_ANALYSIS_H
#define DATA_FLOW_ANALYSIS_H

#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Casting.h>
#include <llvm/ADT/Optional.h>
#include <set>
#include <map>
#include <string>
#include "Analyzer.h"
#include "Common.h"
#include "PathSpan.h"

class AbstractCondition {
public:
    enum ShapeKind {
        SK_AC,
        SK_FB,
    };
private:
    const ShapeKind shapeKind;
public:
    ShapeKind getKind() const { return shapeKind; }

    explicit AbstractCondition(ShapeKind shapeKind, const Value* lhs, const Instruction* origin, bool fromConditionalBranch)
        : shapeKind(shapeKind)
        , lhs(lhs)
        , origin(origin)
        , fromConditionalBranch(fromConditionalBranch) {}

    virtual ~AbstractCondition() = default;

    inline const Value* getLhs() const {
        return lhs;
    }

    inline const BasicBlock* getParent() const {
        return origin->getParent();
    }

    inline const Instruction* getOrigin() const {
        return origin;
    }

    inline bool isFromConditionalBranch() const {
        return fromConditionalBranch;
    }

    virtual void dump() const = 0;

protected:
    const Value* lhs;
    const Instruction* origin;
    bool fromConditionalBranch;
};

class AbstractComparison : public AbstractCondition {
public:
    explicit AbstractComparison(const ICmpInst* cmp, bool fromConditionalBranch)
            : AbstractCondition(ShapeKind::SK_AC, cmp->getOperand(0), cmp, fromConditionalBranch)
            , predicate(cmp->getPredicate())
            , rhs(cmp->getOperand(1)) {}

    explicit AbstractComparison(ICmpInst::Predicate predicate, const Value* lhs, const Value* rhs, const Instruction* origin, bool fromConditionalBranch)
            : AbstractCondition(ShapeKind::SK_AC, lhs, origin, fromConditionalBranch)
            , predicate(predicate)
            , rhs(rhs) {}

    ~AbstractComparison() override = default;

    void dump() const override;

    inline const Value* getRhs() const {
        return rhs;
    }

    inline ICmpInst::Predicate getPredicate() const {
        return predicate;
    }

    inline ICmpInst::Predicate getInversePredicate() const {
        return ICmpInst::getInversePredicate(predicate);
    }

    static bool classof(const AbstractCondition* C) {
        return C->getKind() == SK_AC;
    }

private:
    ICmpInst::Predicate predicate;
    const Value* rhs;
};

class AbstractFallback : public AbstractCondition {
public:
    explicit AbstractFallback(const Value* lhs, const Instruction* origin)
        : AbstractCondition(ShapeKind::SK_FB, lhs, origin, true) {}

    ~AbstractFallback() override = default;

    void dump() const override;

    static bool classof(const AbstractCondition* C) {
        return C->getKind() == SK_FB;
    }
};

using namespace llvm;

using PHISet = SmallPtrSet<const Value*, 2>;

class DataFlowAnalysis {
	public:
		explicit DataFlowAnalysis(GlobalContext *GCtx [[maybe_unused]]) {}
		~DataFlowAnalysis() = default;

        static llvm::Optional<int> computeRhs(const AbstractComparison& cmp);
        static llvm::Optional<int> computeRhsFromValue(const Value* rhsValue);

        static void getLinearUniquePathBackwards(const BasicBlock* currentBlock, vector<const BasicBlock*>& blocks);
        static void getLinearUniquePathForwards(const BasicBlock* currentBlock, vector<const BasicBlock*>& blocks);

        static const Value* findUndisputedValueWithoutLeavingCurrentPath(const Value* value, const Instruction* originPointFromWhereToLookBack, PathSpan blocks, PHISet& phiSet);
        static const Value* findUndisputedValueWithoutLeavingCurrentPathResolveLoad(const LoadInst* load, PathSpan blocks, PHISet& phiSet);

        static void getPotentialSanityCheck(const BasicBlock& BB, const std::function<void(const AbstractCondition*, pair<const Value*, unsigned int>)>& callback);

        static void collectCalls(const Value* V, set<const Value*>& visited, set<const Value*>& result);
};

#endif

