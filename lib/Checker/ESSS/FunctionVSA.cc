#include <llvm/IR/Instructions.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/InstIterator.h>
#include <map>
#include "Checker/ESSS/FunctionVSA.h"
#include "Checker/ESSS/Interval.h"
#include "Checker/ESSS/Common.h"
#include "Checker/ESSS/Analyzer.h"
#include "Checker/ESSS/ClOptForward.h"
#include "Checker/ESSS/DataFlowAnalysis.h"
#include "Checker/ESSS/Helpers.h"



// TODO: this uses return value index 0

static const Interval* getUniqueInterval(const FunctionToIntervalCounts& allIntervals, pair<const Function*, unsigned int> index) {
    if (auto it = allIntervals.find(index); it != allIntervals.end()) {
        if (it->second.size() == 1) {
            return &it->second.begin()->first;
        }
    }
    return nullptr;
}

static bool isThereNoWriteToValueType(const Function* function, const Value* value) {
    // This is very conservative, but works properly in most cases
    auto valueDerefType = value->getType()->getPointerElementType();
    for (const auto& instruction : instructions(function)) {
        if (auto store = dyn_cast<StoreInst>(&instruction)) {
            if (valueDerefType == store->getPointerOperand()->getType()->getPointerElementType())
                return false;
        }
    }

    return true;
}

IntervalHashMap FunctionVSA::refine(const Function* function, IntervalHashMap& map) {
    if (function->empty() || !function->getReturnType()->isIntegerTy()) {
        return std::move(map);
    }
    auto functionErrorReturnIntervalsIt = map.begin();
    auto functionErrorReturnIntervalsItEnd = map.end();
    IntervalHashMap result;
    ValueSet valueSet;
    auto constantRange = computeConstantRangeFor(function, valueSet);
    if (VerboseLevel >= LOG_VERBOSE) {
        LOG(LOG_VERBOSE, "VSA for: " << function->getName() << " ");
        // constantRange.dump();
        LOG(LOG_VERBOSE, "\n");
    }
    for (; functionErrorReturnIntervalsIt != functionErrorReturnIntervalsItEnd; ++functionErrorReturnIntervalsIt) {
        auto interval = functionErrorReturnIntervalsIt->first;
        if (!constantRange.isEmptySet() && !constantRange.isFullSet()) {
            Interval tmp(true);
            int64_t lower = constantRange.getLower().getSExtValue();
            int64_t upper = constantRange.getUpper().getSExtValue() - 1;
            int lower32 = std::max(static_cast<int64_t>(INT_MIN), lower);
            int upper32 = std::min(static_cast<int64_t>(INT_MAX), upper);
            if (upper32 < lower32) {
                continue;
            }
            tmp.appendUnsafeBecauseExpectsSortMaintained(Range(lower32, upper32));
            tmp.intersectionInPlace(interval);
#if 0
            LOG(LOG_INFO, "Narrowing " << function->getName() << " from -> to:\n");
            // constantRange.dump();
            // interval.dump();
#endif
            if (map.size() > 1 && tmp == interval) {
                //interval.clear();
            } else {
                interval.replaceWith(std::move(tmp));
            }
        } else if (constantRange.isEmptySet()) {
            interval.clear();
        }

        if (auto it = result.find(interval); it != result.end()) {
            it->second += functionErrorReturnIntervalsIt->second;
        } else {
            result.emplace(std::move(interval), functionErrorReturnIntervalsIt->second);
        }
    }
    return result;
}

ConstantRange FunctionVSA::computeConstantRangeFor(const Value* V, const Instruction* C, ValueSet& valueSet) {
    assert(V && C);
    unsigned int bitWidth = V->getType()->isIntegerTy() ? V->getType()->getIntegerBitWidth() : (V->getType()->isPointerTy() ? 64 : 32);
    assert(bitWidth > 0);
    if (auto ct = dyn_cast<ConstantInt>(V)) {
        if (ct->getSExtValue() == INT_MAX) {
            //return {APInt(bitWidth, ct->getSExtValue()), APInt(bitWidth, ct->getSExtValue())};
            return ConstantRange::getFull(bitWidth);
        } else {
            return {APInt(bitWidth, ct->getSExtValue()), APInt(bitWidth, ct->getSExtValue() + 1)};
        }
    } else if (isa<ConstantPointerNull>(V)) {
        return {APInt(bitWidth, 0), APInt(bitWidth, 1)};
    }

    // Support dirty tricks (like xor 1) on i1's in a generic way, and also support cmp
    if (bitWidth == 1) {
        return ConstantRange::getFull(1);
    }
    if (!valueSet.insert(V).second) {
        return ConstantRange::getEmpty(bitWidth);
    }
    ConstantRange range = ConstantRange::getFull(bitWidth);
    if (auto call = dyn_cast<CallInst>(V)) {
        auto calleesIt = GlobalCtx.Callees.find(call);
        range = ConstantRange::getEmpty(bitWidth);
        if (calleesIt != GlobalCtx.Callees.end() && !calleesIt->second.empty()) {
            for (const auto *callee: calleesIt->second) {
                range = range.unionWith(computeConstantRangeFor(callee, valueSet));
                if (VerboseLevel >= LOG_VERBOSE) {
                    LOG(LOG_INFO,
                        "  Callee: " << C->getFunction()->getName() << " -> " << callee->getName() << "\n");
                    // range.dump();
                    LOG(LOG_INFO, "\n");
                }
                if (range.isFullSet()) // Fast failure path
                    return range;
            }
        } else {
            LOG(LOG_VERBOSE, "No callees\n");
        }
    } else if (isa<Argument>(V)) {
        // Error values must be generated by the function itself, and not passed in
        range = ConstantRange::getEmpty(bitWidth);
    } else if (auto select = dyn_cast<SelectInst>(V)) {
        range = computeConstantRangeFor(select->getTrueValue(), select, valueSet);
        if (range.isFullSet()) // Fast path
            return range;
        range = range.unionWith(computeConstantRangeFor(select->getFalseValue(), select, valueSet));
    } else if (auto phi = dyn_cast<PHINode>(V)) {
        range = ConstantRange::getEmpty(bitWidth);
        for (unsigned int i = 0, l = phi->getNumIncomingValues(); i < l; ++i) {
            ConstantRange valueRange = computeConstantRangeFor(phi->getIncomingValue(i), phi, valueSet);
            if (valueRange.getLower().getSExtValue() > valueRange.getUpper().getSExtValue()) {
                valueRange = ConstantRange{valueRange.getUpper(), valueRange.getLower()};
            }
            if (C->getFunction()->getName().equals("BIO_ctrl")) {
                LOG(LOG_INFO, "ABCD\n");
                // phi->getIncomingValue(i)->dump();
                // valueRange.dump();
                LOG(LOG_INFO, "\n");
                // range.dump();
                LOG(LOG_INFO, "\n");
                LOG(LOG_INFO, "\n");
            }
            range = range.unionWith(valueRange);
            if (range.isFullSet()) { // Fast failure path
                LOG(LOG_INFO, "phi range full set: " << C->getFunction()->getName() << "\n");
                // phi->getIncomingValue(i)->dump();
                // phi->dump();
                return range;
            }
        }
    } else if (auto trunc = dyn_cast<TruncInst>(V)) {
        return computeConstantRangeFor(trunc->getOperand(0), trunc, valueSet).truncate(trunc->getDestTy()->getIntegerBitWidth());
    } else if (auto zext = dyn_cast<ZExtInst>(V)) {
        return computeConstantRangeFor(zext->getOperand(0), zext, valueSet).zeroExtend(zext->getDestTy()->getIntegerBitWidth());
    } else if (auto sext = dyn_cast<SExtInst>(V)) {
        return computeConstantRangeFor(sext->getOperand(0), sext, valueSet).signExtend(sext->getDestTy()->getIntegerBitWidth());
    } else if (auto bop = dyn_cast<BinaryOperator>(V)) {
        auto isDisallowedBop = [](const BinaryOperator *B) {
            auto opcode = B->getOpcode();
            return opcode == Instruction::BinaryOps::Add || opcode == Instruction::BinaryOps::Sub ||
                   opcode == Instruction::BinaryOps::Mul || opcode == Instruction::BinaryOps::SDiv ||
                   opcode == Instruction::BinaryOps::UDiv || opcode == Instruction::BinaryOps::SRem ||
                   opcode == Instruction::BinaryOps::Or;
        };

        if (bop->getOpcode() == Instruction::BinaryOps::And) {
            if (auto constant = dyn_cast<ConstantInt>(bop->getOperand(1))) {
                auto upper = constant->getSExtValue();
                if (upper < 0) {
                    // TODO
                } else {
                    range = ConstantRange(APInt(range.getBitWidth(), 0),
                                          APInt(range.getBitWidth(), constant->getSExtValue() + 1));
                }
            }
        }

        if (isDisallowedBop(bop)) {
            return ConstantRange::getEmpty(bop->getType()->getIntegerBitWidth());
        } else if (auto transitiveBop = dyn_cast<BinaryOperator>(bop->getOperand(0)); transitiveBop && isDisallowedBop(transitiveBop)) {
            // Or transitively dependent on bop
            return ConstantRange::getEmpty(transitiveBop->getType()->getIntegerBitWidth());
        }

        // TODO: approximate other bitwise ops?

        // In other cases, bail
    } else if (auto load = dyn_cast<LoadInst>(V)) {
        auto aaIt = GlobalCtx.AAPass.find(load->getModule());
        assert(aaIt != GlobalCtx.AAPass.end());
        range = ConstantRange::getEmpty(range.getBitWidth());
        bool has = false;
        for (const auto& instruction : instructions(C->getFunction())) {
            if (auto store = dyn_cast<StoreInst>(&instruction)) {
                if (aaIt->second->getAAResults().alias(load->getPointerOperand(), store->getPointerOperand()) >= AliasResult::PartialAlias) {
                    has = true;
                    range = range.unionWith(computeConstantRangeFor(store->getValueOperand(), store, valueSet).sextOrTrunc(bitWidth));
                }
            }
        }
        if (!has) {
            if (auto gep = dyn_cast<GetElementPtrInst>(load->getPointerOperand())) {
                auto strippedValue = gep->stripPointerCastsAndAliases();
                if (auto instruction = dyn_cast<Instruction>(strippedValue)) {
                    if (auto callees = getCalleeIteratorForPotentialCallInstruction(GlobalCtx, *instruction)) {
                        for (const auto *callee: callees.value()->second) {
                            if (!isThereNoWriteToValueType(callee, strippedValue)) {
                                return ConstantRange::getFull(range.getBitWidth());
                            }
                        }
                        return ConstantRange::getEmpty(range.getBitWidth());
                    } else {
                        return ConstantRange::getEmpty(range.getBitWidth());
                    }
                }
            }
            return ConstantRange::getFull(range.getBitWidth());
        }
    } else {
#if 1
        LOG(LOG_INFO, "Constant range inference failed: " << C->getFunction()->getName() << "\n");
        // V->dump();
#endif
    }

    return range;
}

ConstantRange FunctionVSA::computeConstantRangeFor(const Function* function, ValueSet& valueSet) {
    if (/*function->empty() || */!function->getReturnType()->isIntegerTy())
        return ConstantRange::getFull(32);

    auto functionToRangeIt = functionToRange.find(function);
    if (functionToRangeIt != functionToRange.end()) {
        //LOG(LOG_INFO, "Cached\n");
        return functionToRangeIt->second;
    }
    ConstantRange constantRange(function->getReturnType()->getIntegerBitWidth(), function->empty());

    // This loop will be ignored for empty functions
    for (auto &BB: *function) {
        if (auto ret = dyn_cast<ReturnInst>(BB.getTerminator())) {
            ConstantRange range = computeConstantRangeFor(ret->getReturnValue(), ret, valueSet);
            constantRange = constantRange.unionWith(range, ConstantRange::PreferredRangeType::Smallest);
        }
    }

    if (/*constantRange.isEmptySet() || */constantRange.isFullSet()) {
        auto uniqueInterval = getUniqueInterval(allIntervals, make_pair(function, 0));
        LOG(LOG_INFO, "unique interval case for full-set of: " << function->getName() << "\n");
        if (uniqueInterval) {
            constantRange = ConstantRange(APInt(constantRange.getBitWidth(), uniqueInterval->lowest()),
                                          APInt(constantRange.getBitWidth(),
                                                uniqueInterval->highest() < INT_MAX ? static_cast<long>(uniqueInterval->highest()) + 1L : INT_MAX));
            // constantRange.dump();
        }
    }

    auto it = functionToRange.find(function);
    if (it == functionToRange.end()) {
        functionToRange.emplace(function, constantRange);
    } else {
        LOG(LOG_INFO, "RANGE: ");
        it->second = constantRange;
        // functionToRange.find(function)->second.dump();
        LOG(LOG_INFO, "\n");
    }
    return constantRange;
}
