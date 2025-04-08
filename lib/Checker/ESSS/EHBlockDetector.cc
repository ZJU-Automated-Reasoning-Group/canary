#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/CFG.h>
#include <llvm/Analysis/CallGraph.h>
#include <stack>

#include "Checker/ESSS/EHBlockDetector.h"
#include "Checker/ESSS/Common.h"
#include "Checker/ESSS/Helpers.h"
#include "Checker/ESSS/DebugHelpers.h"
#include "Checker/ESSS/DataFlowAnalysis.h"
#include "Checker/ESSS/Interval.h"
#include "Checker/ESSS/ClOptForward.h"
#include "Checker/ESSS/FunctionVSA.h"


//#define VERY_VERBOSE_DUMP_OF_MATCH
//#define DUMP_SUMMARIES
//#define DUMP_PATHS
#define DUMP_CONFIDENCE_INFO


vector<const BasicBlock*> extendPathWithUniquePredecessors(const Path* path) {
    vector<const BasicBlock*> blocksCopy;
    {
        auto begin = path->blocks[0];
        auto previous = begin->getUniquePredecessor();
        while (previous) {
            blocksCopy.insert(blocksCopy.begin(), previous);
            previous = previous->getUniquePredecessor();
        }
        blocksCopy.reserve(path->blocks.size());
        blocksCopy.insert(blocksCopy.end(), path->blocks.begin(), path->blocks.end());
    }

    // Prevent cycle
    if (blocksCopy.size() >= 2 && *blocksCopy.begin() == blocksCopy.back()) {
        blocksCopy.pop_back();
    }

    return blocksCopy;
}

unsigned int Summary::numberOfCondBrs() const {
    return count_if(ops, [](const Operation& operation) {
        return operation.type == OperationType::CondBr;
    });
}

void Summary::dump() const {
    for (const auto& entry : ops) {
        if (entry.type == OperationType::Call) {
            LOG(LOG_INFO, "    CALL [set=" << entry.callTargets << "] ");
            if (entry.callTargets) {
                auto end = entry.callTargets->end();
                for (auto it = entry.callTargets->begin(); it != end;) {
                    auto* target = *it;
                    LOG(LOG_INFO, target->getName());
                    ++it;
                    if (it != end) {
                        LOG(LOG_INFO, ", ");
                    }
                }
                LOG(LOG_INFO, "\n");
            } else {
                LOG(LOG_INFO, "?\n");
            }
        } else if (entry.type == OperationType::Return) {
            LOG(LOG_INFO, "    RETURN ");
            if (entry.returnData.resolvedValue) {
                // entry.returnData.resolvedValue->dump();
                LOG(LOG_INFO, "(has resolved value)");
            } else {
                LOG(LOG_INFO, "(no resolved value)");
            }
            if (entry.returnData.unresolvedValue) {
                // entry.returnData.unresolvedValue->dump();
                LOG(LOG_INFO, "(has unresolved value)");
            } else {
                LOG(LOG_INFO, "(null)");
            }
            LOG(LOG_INFO, "\n");
        } else if (entry.type == OperationType::Unreachable) {
            LOG(LOG_INFO, "UNREACHABLE\n");
        } else if (entry.type == OperationType::CondBr) {
            LOG(LOG_INFO, "    CONDBR " << entry.predicate << " " << entry.condBrData.instruction << " " << entry.condBrData.value << " ");
            if (entry.condBrData.value)
                // entry.condBrData.value->dump();
                LOG(LOG_INFO, "(has value)\n");
            else
                LOG(LOG_INFO, "\n");
        } else if (entry.type == OperationType::Switch) {
            LOG(LOG_INFO, "    SWITCH\n");
        } else if (entry.type == OperationType::Store) {
            LOG(LOG_INFO, "    STORE ");
            if (entry.storeData.value)
                // entry.storeData.value->dump();
                LOG(LOG_INFO, "(has store value)");
            LOG(LOG_INFO, "\n");
        } else {
            assert(false && "unimplemented");
        }
    }
}

static bool areCallsEquivalent(const FuncSet* first, const FuncSet* second) {
    // NOTE: maybe we can leverage some kind of deduplication logic to do fast compares and save memory?
    // Fast paths
    if (first == second)
        return true;
    if (first->size() != second->size())
        return false;

    // Slow path
    return all_of(*first, [second](const Function* target) {
        return second->find(target) != second->end();
    });
}

static bool areInlinedEquivalent(const Instruction* a, const Instruction* b) {
    auto aLoc = a->getDebugLoc().get();
    auto bLoc = b->getDebugLoc().get();
    if (!aLoc || !bLoc) return false;
    if (!aLoc->getInlinedAt() || !bLoc->getInlinedAt()) return false;
    if (aLoc->getLine() == 0 || bLoc->getLine() == 0) return false;
    if (aLoc->getLine() == bLoc->getLine() && aLoc->getColumn() == bLoc->getColumn()) {
        return true;
    } else {
        return true;
    }
}

bool Operation::operator==(const Operation& other) const {
    if (type != other.type)
        return false;
    if (type == OperationType::Call)
        return areCallsEquivalent(callTargets, other.callTargets);
    else if (type == OperationType::CondBr) {
        if (predicate == other.predicate && condBrData.value && other.condBrData.value) {
            if (isa<GetElementPtrInst>(condBrData.value) && isa<GetElementPtrInst>(other.condBrData.value))
                return false; // Resolution independent of path
            if (condBrData.value == other.condBrData.value)
                return true;
            auto valueAsCall = dyn_cast<CallInst>(condBrData.value);
            auto otherValueAsCall = dyn_cast<CallInst>(other.condBrData.value);
            if (valueAsCall && otherValueAsCall) {
                auto valueTargets = GlobalCtx.Callees.find(valueAsCall);
                auto otherValueTargets = GlobalCtx.Callees.find(otherValueAsCall);
                if (valueTargets != GlobalCtx.Callees.end() && otherValueTargets != GlobalCtx.Callees.end()) {
                    return areCallsEquivalent(&valueTargets->second, &otherValueTargets->second);
                }
            }
        }
        return false;
    } else if (type == OperationType::Return) {
        if ((returnData.resolvedValue == nullptr) != (other.returnData.resolvedValue == nullptr))
            return false;
        if (!returnData.resolvedValue && !other.returnData.resolvedValue) {
            return returnData.unresolvedValue == other.returnData.unresolvedValue;
        } else {
            if (isa<CallInst>(returnData.resolvedValue) && isa<CallInst>(other.returnData.resolvedValue)) {
                // NOTE: we can only be certain it's the same value if the called operand matches exactly.
                //       So we can't really compare indirect sets.
                //return dyn_cast<CallInst>(returnData.resolvedValue)->getCalledOperand() == dyn_cast<CallInst>(other.returnData.resolvedValue)->getCalledOperand();

                // NOTE: a more lenient variation: if they're the same phi node for example it can also be fine to match them (early return-style programming)
                return returnData.unresolvedValue == other.returnData.unresolvedValue;
            }
            if ((isa<CallInst>(returnData.resolvedValue) && isa<ConstantInt>(other.returnData.resolvedValue)) || (isa<CallInst>(other.returnData.resolvedValue) && isa<ConstantInt>(returnData.resolvedValue)))
                return true;

            return returnData.resolvedValue == other.returnData.resolvedValue;
        }
    }
    else if (type == OperationType::Store) {
        if (!storeData.value || !other.storeData.value)
            return false;
        const Module* module;
        if (auto ins = dyn_cast<Instruction>(storeData.value))
            module = ins->getModule();
        else if (auto arg = dyn_cast<Argument>(storeData.value))
            module = arg->getParent()->begin()->getModule();
        else
            return storeData.value == other.storeData.value;
        if (GlobalCtx.AAPass.find(module)->second->getAAResults().alias(storeData.value, other.storeData.value) >= AliasResult::MayAlias)
            return true;
        return areInlinedEquivalent(storeData.instruction, other.storeData.instruction);
    } else if (type == OperationType::Switch || type == OperationType::Unreachable)
        return true;
    assert(false && "not implemented");
}

static const Value* resolveValueAlongPath(const Value* value, const vector<const BasicBlock*>& blocks) {
    // NOTE: if this function returns an instruction of operation, it means the return value is independent
    //       of the taken path. So it should be fine to cross-match.
    if (!value)
        return nullptr;
    if (isa<Constant>(value) || isa<Argument>(value))
        return value;
    auto instruction = dyn_cast<Instruction>(value);
    assert(instruction);
    PHISet phiSet;
    return DataFlowAnalysis::findUndisputedValueWithoutLeavingCurrentPath(value, instruction, PathSpan{blocks, false}, phiSet);
}

bool Operation::resolvePathSensitiveValues(const vector<const BasicBlock*>& blocks) {
    if (type == OperationType::Store) {
        value = resolveValueAlongPath(value, blocks);
    } else if (type == OperationType::Return) {
        returnData.resolvedValue = resolveValueAlongPath(returnData.unresolvedValue, blocks);
    } else if (type == OperationType::CondBr) {
        condBrData.value = resolveValueAlongPath(condBrData.value, blocks);
        if (!condBrData.value)
            condBrData.value = condBrData.instruction;
    }
    return true;
}

bool Summary::resolvePathSensitiveValues(const vector<const BasicBlock*>& blocks) {
    for (auto& op : ops)
        if (!op.resolvePathSensitiveValues(blocks)) // If fatal error
            return false;
    return true;
}

template<typename T>
static unsigned short longestCommonSubsequence(const T* a, size_t aLen, const T* b, size_t bLen) {
    // To reduce memory usage, we should pick the smallest array for the rows
    if (aLen < bLen) {
        swap(aLen, bLen);
        swap(a, b);
    }

    auto* currentRow = new unsigned short[bLen + 1];
    currentRow[0] = 0;
    auto* previousRow = new unsigned short[bLen + 1]();

    for (size_t i = 1; i <= aLen; ++i) {
        for (size_t j = 1; j <= bLen; ++j) {
            if (a[i - 1] == b[j - 1]) {
                currentRow[j] = previousRow[j - 1] + 1;
            } else {
                currentRow[j] = max(currentRow[j - 1], previousRow[j]);
            }
        }
        swap(currentRow, previousRow);
    }

    auto ret = previousRow[bLen];

    delete[] currentRow;
    delete[] previousRow;

    return ret;
}

template<typename T>
static unsigned short longestCommonSubsequence(const vector<T>& a, const vector<T>& b) {
    return longestCommonSubsequence(a.data(), a.size(), b.data(), b.size());
}

template<typename T>
inline static float longestCommonSubsequenceRatio(const vector<T>& a, const vector<T>& b, unsigned short* lcsOut) {
    auto lcs = longestCommonSubsequence(a, b);
    *lcsOut = lcs;
    return static_cast<float>(lcs) / min(a.size(), b.size());
}

Summary EHBlockDetectorPass::summarizeBlock(const BasicBlock* currentBlock) const {
    Summary summary;

    for (const auto& instruction : *currentBlock) {
        if (auto CI = dyn_cast<CallInst>(&instruction)) {
            if (CI->isDebugOrPseudoInst() || CI->isLifetimeStartOrEnd() || CI->isInlineAsm())
                continue;

            // Ignore branch conditions, see @generate_unverifiable_g
            if (any_of(CI->users(), [](const User* user) {
                if (auto cmp = dyn_cast<CmpInst>(user)) {
                    for (auto cmpUser : cmp->users()) {
                        if (auto br = dyn_cast<BranchInst>(cmpUser); br && br->isConditional())
                            return true;
                    }
                }
                return false;
            })) {
                continue;
            }

            if (auto calleesIt = Ctx->Callees.find(CI); calleesIt != Ctx->Callees.end()) {
                summary.ops.emplace_back(Operation {
                    .type = OperationType::Call,
                    .callTargets = &calleesIt->second,
                });
            } else {
                // CI->dump();
                SourceLocation{CI}.dump(LOG_INFO);
                LOG(LOG_INFO, '\n');
                continue;
            }
        } else if (auto ret = dyn_cast<ReturnInst>(&instruction)) {
            summary.ops.emplace_back(Operation {
                .type = OperationType::Return,
                .returnData = {
                    .resolvedValue = nullptr,
                    .unresolvedValue = ret->getReturnValue(),
                }
            });
        } else if (auto condBr = dyn_cast<BranchInst>(&instruction)) {
            if (condBr->isConditional()) {
                auto predicate = ICmpInst::Predicate::ICMP_EQ;
                auto value = condBr->getOperand(0);
                if (auto icmp = dyn_cast<ICmpInst>(value)) {
                    value = icmp->getOperand(0);
                    predicate = icmp->getPredicate();
                }

                summary.ops.emplace_back(Operation {
                    .type = OperationType::CondBr,
                    .predicate = predicate,
                    .condBrData = {
                        .value = value,
                        .instruction = condBr,
                    }
                });
            }
        } else if (auto store = dyn_cast<StoreInst>(&instruction)) {
            summary.ops.emplace_back(Operation{
                    .type = OperationType::Store,
                    .storeData = {
                            .value = store->getPointerOperand(),
                            .instruction = store,
                    },
            });
        } else if (isa<SwitchInst>(instruction)) {
            summary.ops.emplace_back(Operation{
                    .type = OperationType::Switch,
            });
        } else if (isa<UnreachableInst>(instruction)) {
            summary.ops.emplace_back(Operation{
                    .type = OperationType::Unreachable,
            });
        }
    }

    return summary;
}

void EHBlockDetectorPass::collectPaths(const BasicBlock* currentBlock, vector<Path*>& allPaths, Path* myCurrentPath, set<const BasicBlock*>& basicBlocksOfNonInterest) {
    set<const BasicBlock*> visited;
    collectPathsAux(currentBlock, allPaths, myCurrentPath, visited, basicBlocksOfNonInterest, nullptr);
}

void EHBlockDetectorPass::collectPathsAux(const BasicBlock* currentBlock, vector<Path*>& allPaths, Path* myCurrentPath, set<const BasicBlock*>& visited, set<const BasicBlock*>& basicBlocksOfNonInterest, const BasicBlock* lastBr) {
    if (!visited.insert(currentBlock).second)
        return;

    myCurrentPath->blocks.push_back(currentBlock);

    if (myCurrentPath->blocks.size() > 1 && basicBlocksOfNonInterest.find(currentBlock) != basicBlocksOfNonInterest.end())
        return;

    // Current block stops function, stop here
    if (isa<ReturnInst>(currentBlock->getTerminator()) || isa<UnreachableInst>(currentBlock->getTerminator()))
        return;

    // Continue path
    if (auto uniqueSuccessor = currentBlock->getUniqueSuccessor()) {
        collectPathsAux(uniqueSuccessor, allPaths, myCurrentPath, visited, basicBlocksOfNonInterest, lastBr);
    }
    // Fork paths
    else {
        auto numberOfSuccessors = succ_size(currentBlock);

        // We don't want to consider double conditions that could stray away from the error path.
        // However, we want to be able to handle the case of AND/OR checks.
        // We should thus only do this if we are not in an "AND case" / "OR case".
        if (numberOfSuccessors > 1) {
            if (!lastBr)
                lastBr = currentBlock;
            else {
                // Common successor is a sign of such cases.
                if (!any_of(successors(currentBlock), [lastBr](const BasicBlock* BB) {
                    return find(successors(lastBr), BB) != succ_end(lastBr);
                })) {
                    return;
                }
            }
        }

        unsigned int currentSuccessor = 0;
        for (const auto *successor : successors(currentBlock)) {
            set<const BasicBlock*> visitedClone{visited};
            if (currentSuccessor == numberOfSuccessors - 1) {
                // Last one can reuse the memory
                collectPathsAux(successor, allPaths, myCurrentPath, visitedClone, basicBlocksOfNonInterest, lastBr);
            } else {
                auto newCurrentPath = new Path();
                newCurrentPath->reason = myCurrentPath->reason;
                newCurrentPath->blocks.reserve(myCurrentPath->blocks.size());
                newCurrentPath->blocks.insert(newCurrentPath->blocks.end(), myCurrentPath->blocks.begin(), myCurrentPath->blocks.end());
                allPaths.push_back(newCurrentPath);
                collectPathsAux(successor, allPaths, newCurrentPath, visitedClone, basicBlocksOfNonInterest, lastBr);
            }
            ++currentSuccessor;
        }
    }
}

void EHBlockDetectorPass::identifyPotentialSanityChecks(const Function& function) {
    for (const auto& BB : function) {
        auto handle = [&](const AbstractCondition* cmp, pair<const Value*, unsigned int> value) {
#if 0
            LOG(LOG_INFO, "Register\n");
            cmp->dump();
            value->dump();
#endif
            if (auto CI = dyn_cast<CallInst>(cmp->getLhs()); CI && (CI->getIntrinsicID() != Intrinsic::not_intrinsic || CI->isInlineAsm())) {
                delete cmp;
                return;
            }
            Ctx->functionToSanityValuesAndConditions[&function].emplace_back(make_pair(value, cmp));
        };
        DataFlowAnalysis::getPotentialSanityCheck(BB, handle);
    }
}

bool EHBlockDetectorPass::doInitialization(Module *M) {
    if (stage != 0)
        return false;

    auto functionPassManager = new legacy::FunctionPassManager(M);
    Ctx->AAPass[M] = new AAResultsWrapperPass();
    functionPassManager->add(Ctx->AAPass[M]);
    functionPassManager->doInitialization();
    for (const auto &function: *M) {
        if (!Ctx->shouldSkipFunction(&function)) {
            functionPassManager->run(const_cast<Function &>(function));
        }
    }

    functionPassManager->doFinalization();

    for (const auto& function : *M) {
        if (!Ctx->shouldSkipFunction(&function))
            identifyPotentialSanityChecks(function);
    }

    moduleToSafetyChecks[M]; // Trick to moduleToSafetyChecks[M]

    return false;
}

bool EHBlockDetectorPass::doFinalization(Module *M) {
    return false;
}

void EHBlockDetectorPass::storeData() {
    /* Merge compatible intervals and pick the strictest one
     * e.g.
     * Function: shmctl
	 *   [-2147483648, -1]
	 *     Confidence: 33.33%, 1/3
	 *   [0, 0]
	 *     Confidence: 33.33%, 1/3
	 *   [-2147483648, -1] U [1, 2147483647]
	 *     Confidence: 33.33%, 1/3
     *
     * shall become
     * Function: shmctl
	 *   [-2147483648, -1]
	 *     Confidence: 66.66%, 2/3
	 *   [0, 0]
	 *     Confidence: 33.33%, 1/3
     *
     * This loop is O(n²), but n is the amount of intervals which is generally very small
     */
    if (RefineWithVSA) {
        FunctionVSA fvsa(functionToIntervalCounts);
        auto it = functionToIntervalCounts.begin();
        auto end = functionToIntervalCounts.end();
        for (; it != end; ++it) {
            if (it->first.first->empty())
                continue;

            const Value* previousOne = nullptr;
            if (all_of(*it->first.first, [&](const BasicBlock& BB) {
                if (auto ret = dyn_cast<ReturnInst>(BB.getTerminator())) {
                    if (!isa<ConstantInt>(ret->getReturnValue()) && !isa<ConstantPointerNull>(ret->getReturnValue()))
                        return false;
                    if (!previousOne) {
                        previousOne = ret->getReturnValue();
                        return true;
                    }
                    return ret->getReturnValue() == previousOne;
                }
                return true;
            })) {
                IntervalHashMap hm;
                hm.emplace(Interval{}, 1);
                it->second = std::move(hm);
                continue;
            }

            it->second = fvsa.refine(it->first.first, it->second);
        }
    }
#if 1
    for (auto& [pair, intervalCounts] : functionToIntervalCounts) {
        auto it = intervalCounts.begin();
        auto end = intervalCounts.end();
        while (it != end) {
            if (it->second > 0) {
                // Find compatible interval
                for (auto itCopy = intervalCounts.begin(); itCopy != end; ++itCopy) {
                    if (pair.first->empty()) {
                        // Remove duplicates that were already merged
                        if (itCopy != it && itCopy->second > 0 && itCopy->second <= it->second && it->first.isSubsetOf(itCopy->first)) {
                            it->second += itCopy->second;
                            itCopy->second = 0;
                        }
                    } else {
                        if (itCopy != it && itCopy->second > 0 && itCopy->second <= it->second && itCopy->first.isSubsetOf(it->first)) {
                            it->second += itCopy->second;
                        }
                    }
                }
            }

            ++it;
        }
    }
#endif

    // Use association analysis to compute intervals, outlier-based on confidence
    for (auto& [pair, intervalCounts] : functionToIntervalCounts) {
        auto function = pair.first;

#ifdef DUMP_CONFIDENCE_INFO
        LOG(LOG_INFO, "Function: " << function->getName() << "\n");
#endif

        float sum = 0;
        for (const auto& [_, count] : intervalCounts) {
            sum += static_cast<float>(count);
        }
        bool intersected = false;
        auto intervalCountsEnd = intervalCounts.end();
        auto intervalCountsIt = intervalCounts.begin();
        float highestFraction = 0.0f;
        while (intervalCountsIt != intervalCountsEnd) {
            const auto& [interval, count] = *intervalCountsIt;

            float fraction = static_cast<float>(count) / sum;
            highestFraction = std::max(fraction, highestFraction);

#ifdef DUMP_CONFIDENCE_INFO
            LOG(LOG_INFO, "\t");
            interval.dump();
            LOG(LOG_INFO, "\tConfidence: " << format("%.2f", fraction * 100.0f) << "%, " << count << "/" << (unsigned int) sum << "\n");
#endif

            if (fraction >= IntervalConfidenceThreshold) {
                auto& targetInterval = Ctx->functionErrorReturnIntervals.intervalFor(pair);
                targetInterval.intersectionInPlace(interval);
                intersected = true;
                ++intervalCountsIt;
            } else {
                intervalCountsIt = intervalCounts.erase(intervalCountsIt);
            }
        }

        if (!intersected) {
            LOG(LOG_VERBOSE, "Dropped " << function->getName() << " due to low confidence, falling back to empty interval\n");
            // Empty interval indicates it must be checked, but not how. The following line creates an empty interval.
            Ctx->functionErrorReturnIntervals.intervalFor(pair).clear();
        }

        // Note: we still need this data to compare support and confidence in case of inheriting error intervals
        Ctx->functionToConfidenceMutex.lock();
        Ctx->functionToConfidence[pair] = highestFraction;
        Ctx->functionToConfidenceMutex.unlock();
    }
    functionToIntervalCounts.clear();
    // Remove transitive empty's
#if 1
    for (const auto& [pair, interval] : Ctx->functionErrorReturnIntervals) {
        if (interval.empty()) {
            auto callersIt = Ctx->Callers.find(pair.first);
            if (callersIt == Ctx->Callers.end())
                continue;
            for (const auto* caller : callersIt->second) {
                auto callerFunction = caller->getFunction();
                if (callerFunction->size() == 1) {
                    if (auto ret = dyn_cast<ReturnInst>(callerFunction->begin()->getTerminator())) {
                        if (auto call = dyn_cast_or_null<CallInst>(ret->getReturnValue())) {
                            if (call->getCalledFunction() == pair.first) {
                                Ctx->functionErrorReturnIntervals.intervalFor(make_pair(callerFunction, 0), true).clear();
                                LOG(LOG_INFO, "Empty " << callerFunction->getName() << "\n");
                            }
                        }
                    }
                }
            }
        }
    }
#endif
}

void EHBlockDetectorPass::associationAnalysisForErrorHandlers() {
    // Use association analysis to compute likely error handling functions, outlier-based on confidence
    for (const auto& [function, counts] : functionToInErrorNotInErrorPair) {
        auto maybeInterval = Ctx->functionErrorReturnIntervals.maybeIntervalFor(make_pair(function, 0));
        if (maybeInterval.has_value())
            continue;

        auto support = counts.inError;
        if (support < 10)
            continue;
        float fraction = static_cast<float>(support) / static_cast<float>(support + counts.notInError);
        if (fraction < AssociationConfidence)
            continue;

        LOG(LOG_INFO, "Function: " << function->getName() << "\n");
        LOG(LOG_INFO, "\tin-error: " << counts.inError << ", not-in-error: " << counts.notInError << ", fraction: " << format("%.2f", fraction * 100.0f) << "%\n");

        associatedErrorHandlerFunctions.insert(function);
    }
    functionToInErrorNotInErrorPair.clear();
}

void EHBlockDetectorPass::stage0(Module* M) {
    auto& safetyChecks = moduleToSafetyChecks.find(M)->second;

    auto testCases = getListOfTestCases();

    for (const auto& F : *M) {
        if (Ctx->shouldSkipFunction(&F))
            continue;

        if (testCases.has_value()) {
            if (find(testCases.value(), F.getName()) == testCases.value().end()) {
                continue;
            }
        }

        auto functionToSanityCheckCallAndCmpInstructionsIt = Ctx->functionToSanityValuesAndConditions.find(&F);
        if (functionToSanityCheckCallAndCmpInstructionsIt == Ctx->functionToSanityValuesAndConditions.end())
            continue;

#if 0
        LOG(LOG_INFO, "# Conditionals of interest: " << functionToSanityCheckCallAndCmpInstructionsIt->second.size() << "\n");
#endif

        map<const AbstractComparison*, const vector<const BasicBlock*>*> conditionalToErrorPath;

        // Collect summarised paths to a terminator of the function
        set<const BasicBlock*> basicBlocksOfNonInterest;
        for (const auto& [value, conditional] : functionToSanityCheckCallAndCmpInstructionsIt->second) {
            if (auto abstractComparison = dyn_cast<AbstractComparison>(conditional)) {
                if (!abstractComparison->isFromConditionalBranch()) continue;
                basicBlocksOfNonInterest.insert(conditional->getParent());
                assert(value.first);
                conditionalToAction.emplace(abstractComparison, value);
                //LOG(LOG_INFO, "Basic block of non interest: " << getBasicBlockName(conditional->getParent()) << "\n");
            }
        }
        vector<Path*> paths;
        for (const auto& [_, conditional] : functionToSanityCheckCallAndCmpInstructionsIt->second) {
            if (!conditional->isFromConditionalBranch()) continue;

            auto currentPath = new Path();
            currentPath->reason = conditional;
            paths.push_back(currentPath);

            if (auto _switch = dyn_cast<SwitchInst>(conditional->getOrigin())) {
                // Non-default cases are comparisons
                if (auto abstractComparison = dyn_cast<AbstractComparison>(conditional)) {
                    // There are many paths possible, but they all depend on the condition.
                    // If we were to start collecting the paths from the conditional, we would collect the same paths multiple times,
                    // and even ones that aren't possible because they are constrained by the equality on the conditional.
                    // We therefore start on the successors and prepend the paths with the current block.
                    auto caseSuccessor = _switch->findCaseValue(dyn_cast<ConstantInt>(abstractComparison->getRhs()))->getCaseSuccessor();
                    currentPath->blocks.push_back(conditional->getParent());
                    collectPaths(caseSuccessor, paths, currentPath, basicBlocksOfNonInterest);
                }
                // Default case is a fallback
                else {
                    currentPath->blocks.push_back(conditional->getParent());
                    collectPaths(_switch->getDefaultDest(), paths, currentPath, basicBlocksOfNonInterest);
                }
            } else {
                // There are only two paths possible: either the branch is taken, or it is not taken.
                collectPaths(conditional->getParent(), paths, currentPath, basicBlocksOfNonInterest);
            }
        }

#ifdef DUMP_PATHS
        LOG(LOG_INFO, "Paths\n");
        for (const auto* path : paths) {
            dumpPathList(path->blocks);
        }
#endif

        vector<const Path*> pathSummaryIndexToPath;
        vector<Summary> pathsAsSummaries;
        DenseMap<const BasicBlock*, Summary> summaries;
        for (const auto* path : paths) {
            if (path->blocks.size() == 1) // Optimisation: this will only be the non-conditional part
                continue;

            // Note: first one is not the conditional part
            auto pathIt = path->blocks.begin() + 1;
            Summary summary;
            for (; pathIt != path->blocks.end(); ++pathIt) {
                auto BB = *pathIt;

                auto it = summaries.find(BB);
                if (it == summaries.end()) {
                    auto subSummary = summarizeBlock(BB);
                    summary.merge(subSummary);
                    summaries.try_emplace(BB, std::move(subSummary));
                } else {
                    summary.merge(it->second);
                }
            }
            if (!summary.ops.empty()) {
                // We must get the longest path leading to this one to improve resolving values.
                auto originalBlockIndex1 = path->blocks[1];
                auto blocksCopy = extendPathWithUniquePredecessors(path);

                if (summary.resolvePathSensitiveValues(blocksCopy)) {
                    summary.originalBlockIndex1 = originalBlockIndex1;
                    pathsAsSummaries.emplace_back(std::move(summary));
                    pathSummaryIndexToPath.push_back(path);
                }
            }
        }

#ifdef DUMP_SUMMARIES
        if (F.getName().equals("...")) {
            unsigned int idx = 0;
            LOG(LOG_INFO, "---\n");
            for (const auto& summary: pathsAsSummaries) {
                if (auto I = dyn_cast<Instruction>(pathSummaryIndexToPath[idx]->reason->getOrigin())) {
                    LOG(LOG_INFO, "[origin]  -> ");
                    SourceLocation{I}.dump(LOG_INFO);
                    I->dump();
                }
                LOG(LOG_INFO, "Index " << idx << "\n");
                summary.dump();

                dumpPathList(pathSummaryIndexToPath[idx]->blocks);

                LOG(LOG_INFO, "===\n");
                ++idx;
            }
            LOG(LOG_INFO, "---\n");
        }
#endif

        auto amountOfSummaries = pathsAsSummaries.size();

        for (size_t i = 0; i < amountOfSummaries; ++i) {
            for (size_t j = i + 1; j < amountOfSummaries; ++j) {
                // Note: multiple paths may origin from the same point
                if (pathSummaryIndexToPath[i]->reason == pathSummaryIndexToPath[j]->reason) {
#if 0
                    LOG(LOG_INFO, "Reason equality skip: " << i << ", " << j << "\n");
#endif
                    continue;
                }

                auto& pathSummaryI = pathsAsSummaries[i];
                auto& pathSummaryJ = pathsAsSummaries[j];

                size_t indicesArray[] { i, j };
                unsigned short lcs;
                float ratio = longestCommonSubsequenceRatio(pathSummaryI.ops, pathSummaryJ.ops, &lcs);

                if (ratio >= 0.999f /* ratio must be 1 */) {
                    auto sumOfCondBrCount = pathSummaryI.numberOfCondBrs() + pathSummaryJ.numberOfCondBrs(); // Penalize on number of condbrs

                    // NOTE: we want to get the longest match for the error handling block because we are more confident in long matches.
                    for (auto pathIdx : indicesArray) {
                        const auto& path = pathSummaryIndexToPath[pathIdx];
                        auto reasonAsComparison = dyn_cast<AbstractComparison>(path->reason);
                        if (!reasonAsComparison) continue;

                        auto pathLength = static_cast<unsigned short>(path->blocks.size()); // Narrowing seems fine, who's going to have more than 64K blocks in their path slices?
                        auto& data = safetyChecks[reasonAsComparison];
                        // NOTE: On the one hand, we want error paths to be the shorter ones, but longer paths provide stronger evidence...
                        if ((data.sumOfCondBrCount > sumOfCondBrCount)
                            || (data.lcs < lcs && data.sumOfCondBrCount >= sumOfCondBrCount)
                            || (data.lcs <= lcs && data.pathLength >= pathLength && data.sumOfCondBrCount >= sumOfCondBrCount)) {
                            data.errorHandlingBlock = pathsAsSummaries[pathIdx].originalBlockIndex1;
                            data.lcs = lcs;
                            data.ratio = ratio;
                            data.pathLength = pathLength;
                            data.sumOfCondBrCount = sumOfCondBrCount;
                            conditionalToErrorPath[reasonAsComparison] = &path->blocks;

                            //LOG(LOG_INFO, "Register as error handling block: " << getBasicBlockName(data.errorHandlingBlock)<<"\n");

#ifdef VERY_VERBOSE_DUMP_OF_MATCH
                            if (debug) {
                                LOG(LOG_INFO, "OVERWRITE: ");
                                reasonAsComparison->dump();
                            }
#endif
                        } else {
#ifdef VERY_VERBOSE_DUMP_OF_MATCH
                            if (debug) {
                                LOG(LOG_INFO, "DONT OVERWRITE: ");
                                reasonAsComparison->dump();
                            }
#endif
                        }
                    }

#ifdef VERY_VERBOSE_DUMP_OF_MATCH
                    if (debug) {
                        for (auto pathIdx: indicesArray) {
                            LOG(LOG_INFO, "---\n");
                            LOG(LOG_INFO, "Path: ");
                            dumpPathList(pathSummaryIndexToPath[pathIdx]->blocks);
                            pathSummaryIndexToPath[pathIdx]->reason->dump();
                            LOG(LOG_INFO, "\n");
                            pathsAsSummaries[pathIdx].dump();
                        }
                        LOG(LOG_INFO, "---\n");
                    }
#endif
                }
            }
        }

        // Register function calls in error paths and not in error paths to perform association analysis
        // First determine the error blocks.
        set<const BasicBlock*> errorBlocks, blocksThatAreOnAtLeastOneInspectedPath;
        for (const auto* path : pathSummaryIndexToPath) {
            if (safetyChecks.find(dyn_cast<AbstractComparison>(path->reason)) == safetyChecks.end())
                continue;

            // Note: first block is the one that contains the condition, so skip that one as it's not part of the error
            //       handling code in the path.
            auto end = path->blocks.end();
            for (auto it = path->blocks.begin() + 1; it != end; ++it) {
                blocksThatAreOnAtLeastOneInspectedPath.insert(*it);
            }
        }
        for (const auto& [reason, path] : conditionalToErrorPath) {
            // Note: first block is the one that contains the condition, so skip that one as it's not part of the error
            //       handling code in the path.
            auto end = path->end();
            for (auto it = path->begin() + 1; it != end; ++it) {
                errorBlocks.insert(*it);
            }
        }
        // Then count
        if (!blocksThatAreOnAtLeastOneInspectedPath.empty()) {
            for (const auto &BB: F) {
                if (blocksThatAreOnAtLeastOneInspectedPath.find(&BB) == blocksThatAreOnAtLeastOneInspectedPath.end())
                    continue;
                auto isErrorBlock = errorBlocks.find(&BB) != errorBlocks.end();
                for (const auto &instruction: BB) {
                    auto calleesIt = getCalleeIteratorForPotentialCallInstruction(*Ctx, instruction);
                    if (calleesIt.has_value()) {
                        for (const auto *target: calleesIt.value()->second) {
                            if (target->isIntrinsic())
                                continue;
                            auto &counts = functionToInErrorNotInErrorPair[target];
                            if (isErrorBlock) {
                                ++counts.inError;
                            } else {
                                ++counts.notInError;
                            }
                        }
                    }
                }
            }
        }

        // Cleanup memory
        for (auto* path : paths)
            delete path;
    }

    processSafetyCheckMapping(safetyChecks);

    if (ShowSafetyChecks) {
        LOG(LOG_INFO, "Safety checks found using similarities:\n");
        for (const auto&[safetyCheckComparison, _]: safetyChecks) {
            safetyCheckComparison->dump();
            LOG(LOG_INFO, "\t");
            SourceLocation{safetyCheckComparison->getOrigin()}.dump(LOG_INFO);
            LOG(LOG_INFO, "\n");
        }
    }
}

void EHBlockDetectorPass::processSafetyCheckMapping(const map<const AbstractComparison*, SafetyCheckData>& mapping) {
    // Take the union for checks on the same value, and intersect for the targets relating to the values.
    map<pair<const CallInst*, unsigned int>, Interval> valueToInterval;
    for (const auto& [safetyCheckComparison, safetyCheckData] : mapping) {
        assert(safetyCheckComparison);
        if (!safetyCheckComparison->isFromConditionalBranch()) continue;
        auto conditionalToActionIt = conditionalToAction.find(safetyCheckComparison);
        assert(conditionalToActionIt != conditionalToAction.end());
        auto pair = conditionalToActionIt->second;
        assert(pair.first);
        auto call = dyn_cast<CallInst>(pair.first);
        if (!call) continue;

        assert(safetyCheckData.errorHandlingBlock != nullptr);

        // Take into account the direction of the branch
        auto predicate = safetyCheckComparison->getPredicate();
        if (auto branch = dyn_cast<BranchInst>(safetyCheckComparison->getParent()->getTerminator())) {
            if (branch->getSuccessor(1) == safetyCheckData.errorHandlingBlock) { // False branch
                predicate = safetyCheckComparison->getInversePredicate();
            }
        }

        auto rhs = DataFlowAnalysis::computeRhs(*safetyCheckComparison);

        // Resolve expression to an interval of error values covered by this expression.
        if (rhs.has_value()) {
            Interval expressionInterval(false);
            expressionInterval.applyInequalityOperator(predicate, rhs.value());
            valueToInterval[make_pair(call, pair.second)].unionInPlace(expressionInterval);
        }
    }
    for (const auto& [pair, interval] : valueToInterval) {
        if (Ctx->Layer1OnlyCalls.find(pair.first) != Ctx->Layer1OnlyCalls.end())
            continue;

        if (auto calleesIt = Ctx->Callees.find(pair.first); calleesIt != Ctx->Callees.end()) {
            for (const auto* target : calleesIt->second) {
                functionToIntervalCounts[make_pair(target, pair.second)][interval]++;
            }
        }
    }
}

const BasicBlock* EHBlockDetectorPass::determineSuccessorOfAbstractComparisonWhichHandlesErrors(const BasicBlock* abstractComparisonBlock) const {
    unsigned int highestCount = 0;
    const BasicBlock* bestSuccessor = nullptr;

    for (const auto* successor : successors(abstractComparisonBlock)) {
        vector<const BasicBlock*> blocks;
        DataFlowAnalysis::getLinearUniquePathForwards(successor, blocks);

        unsigned int currentCount = 0;
        for (const auto* block : blocks) {
            for (const auto &instruction: *block) {
                auto calleesIt = getCalleeIteratorForPotentialCallInstruction(GlobalCtx, instruction);
                if (calleesIt.has_value()) {
                    if (any_of(calleesIt.value()->second, [this](const Function *target) {
                        return associatedErrorHandlerFunctions.find(target) != associatedErrorHandlerFunctions.end();
                    })) {
                        ++currentCount;
                    }
                }
            }
        }

        if (currentCount > highestCount) {
            highestCount = currentCount;
            bestSuccessor = successor;
        }
    }

    return bestSuccessor;
}

const BasicBlock* EHBlockDetectorPass::determineSuccessorOfAbstractComparisonWhichHandlesErrors(const AbstractComparison* abstractComparison) const {
    return determineSuccessorOfAbstractComparisonWhichHandlesErrors(abstractComparison->getParent());
}

/**
 * Detect basic blocks as error blocks if a typical error handling function is called within the block
 */
void EHBlockDetectorPass::stage1(Module* M) {
    auto& safetyChecks = moduleToSafetyChecks[M];

    map<const AbstractComparison*, SafetyCheckData> mapping;
    for (const auto& F : *M) {
        if (F.empty())
            continue;
        auto it = Ctx->functionToSanityValuesAndConditions.find(&F);
        LOG(LOG_VERBOSE, "Stage 1 for " << F.getName() << " with " << (it == Ctx->functionToSanityValuesAndConditions.end() ? 0 : it->second.size()) << " sanity values\n");
        if (it == Ctx->functionToSanityValuesAndConditions.end())
            continue;
        for (const auto& [value, abstractCondition] : it->second) {
            auto abstractComparison = dyn_cast<AbstractComparison>(abstractCondition);
            if (!abstractComparison)
                continue;
            if (safetyChecks.find(abstractComparison) != safetyChecks.end())
                continue;

#if 0
            LOG(LOG_INFO, "Found one:\n");
            value->dump();
            if (auto I = dyn_cast<Instruction>(value)) {
                SourceLocation{I}.dump();
                LOG(LOG_INFO, "\n");
            }
            abstractCondition->dump();
#endif

            if (auto errorHandlingBlock = determineSuccessorOfAbstractComparisonWhichHandlesErrors(abstractComparison)) {
                // Construct safety check data
                SafetyCheckData safetyCheckData {
                        .lcs = 0, // Doesn't matter
                        .pathLength = 0, // Doesn't matter
                        .ratio = 0.0f, // Doesn't matter
                        .errorHandlingBlock = errorHandlingBlock,
                        .sumOfCondBrCount = 0, // Doesn't matter
                };
                mapping.emplace(abstractComparison, safetyCheckData);
                LOG(LOG_VERBOSE, "Discovered a new error handling path starting at: " << getBasicBlockName(errorHandlingBlock) << "\n");
            }
        }
    }

    processSafetyCheckMapping(mapping);
}

optional<Interval> EHBlockDetectorPass::addForSpanAndReturnInstruction(PathSpan pathSpan, const ReturnInst* returnInstruction) {
    PHISet phiSet;
    auto returnValue = DataFlowAnalysis::findUndisputedValueWithoutLeavingCurrentPath(
            returnInstruction->getReturnValue(), returnInstruction, pathSpan, phiSet);
    if (!returnValue) return {};
    auto returnValueConstant = DataFlowAnalysis::computeRhsFromValue(returnValue); // Resolve to constant
    if (returnValueConstant.has_value()) {
        Interval tmp(true);
        tmp.appendUnsafeBecauseExpectsSortMaintained(
                Range(returnValueConstant.value(), returnValueConstant.value()));
        return {std::move(tmp)};
    } else {
        return {};
    }
}

optional<bool> EHBlockDetectorPass::determineErrorBranchOfCallWithCompare(ICmpInst::Predicate predicate, unsigned int returnValueIndex, int rhs, const CallInst* checkedCall) {
    // Compute the intersection of the intervals for all possible callees
    auto calleesIt = getCalleeIteratorForPotentialCallInstruction(GlobalCtx, *checkedCall);
    if (!calleesIt.has_value()) return {};
    optional<Interval> accumulation;
    for (const auto *callee: calleesIt.value()->second) {
        //if (isProbablyPure(callee))
        //    continue;
        auto maybeInterval = GlobalCtx.functionErrorReturnIntervals.maybeIntervalFor(make_pair(callee, returnValueIndex));
        //LOG(LOG_INFO, "Callee: " << callee->getName() << "\n");
        if (!maybeInterval.has_value()) continue;
        //maybeInterval.value()->dump();
        if (accumulation.has_value())
            accumulation.value().intersectionInPlace(*maybeInterval.value());
        else
            accumulation = **maybeInterval;
    }

    // Only continue if we have a non-empty interval
    if (!accumulation.has_value() || accumulation.value().empty()) return {};

    Interval predicateInterval(false);
    predicateInterval.applyInequalityOperator(predicate, rhs);
    predicateInterval.intersectionInPlace(accumulation.value());
    bool trueTakenBranchIsAnError = !predicateInterval.empty();
    return trueTakenBranchIsAnError;
}

static bool returnsOnlyOneConstant(const Function* function) {
    const ConstantInt* constantInt = nullptr;
    return (!function->empty() && all_of(*function, [&](const BasicBlock& BB) {
        if (auto ret = dyn_cast<ReturnInst>(BB.getTerminator())) {
            auto currentConstant = dyn_cast<ConstantInt>(ret->getReturnValue());
            if (!currentConstant)
                return false;
            if (!constantInt) {
                constantInt = currentConstant;
                return true;
            } else {
                return constantInt == currentConstant;
            }
        }
        return true;
    }));
}

/**
 * If we don't yet have error interval information for a function F, learn by doing the following:
 * - Detect basic blocks in F as error blocks if it handles a known error condition from an unmarked safety check
*/
void EHBlockDetectorPass::propagateCheckedErrors() {
    // First, collect the functions that might get a propagation
    stack<const Function*> functionsThatMightGetAPropagation;
    set<const Function*> functionsIveAlreadySeen;

    auto addCallersToMightPropagateSet = [&](const Function* F) {
        auto callersIt = Ctx->Callers.find(F);
        if (callersIt != Ctx->Callers.end()) {
            for (const auto* call : callersIt->second)
                functionsThatMightGetAPropagation.emplace(call->getFunction());
        }
    };

    for (const auto& [pair, interval] : Ctx->functionErrorReturnIntervals) {
        if (interval.empty()) continue;
        auto errorReturningFunction = pair.first;
        addCallersToMightPropagateSet(errorReturningFunction);
    }

    unsigned int iterationNumber = 1;
    while (!functionsThatMightGetAPropagation.empty()) {
        auto functionThatMightGetAPropagation = functionsThatMightGetAPropagation.top();
        functionsThatMightGetAPropagation.pop();
        if (!functionsIveAlreadySeen.insert(functionThatMightGetAPropagation).second)
            continue;

        FunctionErrorReturnIntervals newIntervals;
        //LOG(LOG_INFO, "Function: " << functionThatMightGetAPropagation->getName() << "\n");
        //if (!function.getName().equals("RSA_blinding_on")) continue;

        auto functionKeyPair = make_pair(functionThatMightGetAPropagation, 0 /* For now, we only support return values */);

        if (functionThatMightGetAPropagation->getReturnType()->isVoidTy())
            continue;
        if (Ctx->functionErrorReturnIntervals.maybeIntervalFor(functionKeyPair).has_value())
            continue;
        if (returnsOnlyOneConstant(functionKeyPair.first))
            continue;

        auto& potentialChecks = Ctx->functionToSanityValuesAndConditions.find(functionThatMightGetAPropagation)->second;
        bool added = false;
        set<const BasicBlock *> checksBlocks;
        for (const auto &[valuePair, abstractCondition]: potentialChecks) {
            checksBlocks.insert(abstractCondition->getParent());
        }
        for (const auto &[valuePair, abstractCondition]: potentialChecks) {
            auto abstractComparison = dyn_cast<AbstractComparison>(abstractCondition);
            if (!abstractComparison) continue;

            auto [checkedValue, returnValueIndex] = valuePair;
            auto checkedCall = dyn_cast<CallInst>(checkedValue);
            if (!checkedCall) continue;

            auto rhs = DataFlowAnalysis::computeRhsFromValue(abstractComparison->getRhs());
            if (!rhs.has_value()) continue;

            optional<bool> decision = determineErrorBranchOfCallWithCompare(abstractComparison->getPredicate(), returnValueIndex, rhs.value(), checkedCall);
            if (!decision.has_value())
                continue;
            bool trueTakenBranchIsAnError = decision.value();

            if (trueTakenBranchIsAnError) {
                //LOG(LOG_VERBOSE, "is an error\n");
            } else {
                //LOG(LOG_VERBOSE, "not an error\n");
                // NOTE: we can't be sure the non-taken path means non-error, because there could be other error handling in there!
                continue;
            }

            // TODO: support switch opcode here
            if (auto br = dyn_cast<BranchInst>(abstractComparison->getParent()->getTerminator()); br && br->isConditional()) {
                auto errorBlock = br->getSuccessor(trueTakenBranchIsAnError ? 0 : 1);
                auto nonErrorBlock = br->getSuccessor(trueTakenBranchIsAnError ? 1 : 0);
                // NOTE: instead of taking a copy of checksBlocks, just modify it in-place
                auto didInsertNonErrorBlock = checksBlocks.insert(nonErrorBlock).second;

                vector<Path *> allPaths;
                Path *myCurrentPath = new Path();
                myCurrentPath->blocks.push_back(abstractComparison->getParent());
                allPaths.push_back(myCurrentPath);
                collectPaths(errorBlock, allPaths, myCurrentPath, checksBlocks);

                if (didInsertNonErrorBlock)
                    checksBlocks.erase(nonErrorBlock);

                for (auto *path: allPaths) {
                    auto lastBlock = path->blocks.back();
                    auto returnInstruction = dyn_cast<ReturnInst>(lastBlock->getTerminator());
                    // We don't necessarily have a path slice that terminates in a return (think about slices that are cut short due to other checks).
                    if (!returnInstruction) continue;

                    auto blocksCopy = extendPathWithUniquePredecessors(path);

                    auto result = addForSpanAndReturnInstruction(PathSpan{blocksCopy, false}, returnInstruction);
                    if (result.has_value()) {
                        newIntervals.intervalFor(functionKeyPair, true).unionInPlace(result.value());
                        added = true;
                        delete path;
                    }
                }
            }
            //newIntervals.dump();
        }

        if (added) {
            LOG(LOG_VERBOSE, "Added for: " << functionThatMightGetAPropagation->getName() << " in " << __FUNCTION__ << "\n");
            addCallersToMightPropagateSet(functionThatMightGetAPropagation);
        }

        //newIntervals.dump();
        //LOG(LOG_INFO, "Completed successful iteration number " << iterationNumber << "\n");
        Ctx->functionErrorReturnIntervals.mergeDestructivelyForOther(newIntervals);
        ++iterationNumber;
    }
}

/**
 * If we don't yet have error interval information for a function F, learn by doing the following:
 * - Detects basic blocks in F as error blocks if it calls typical error (a) handling function(s), which is similar
 *   to stage 1, except that in this case we're learning about F and not the function that gets called.
 */
void EHBlockDetectorPass::learnErrorsFromErrorBlocksForSelf() {
    // First, collect the functions that might get a propagation
    stack<const Function*> functionsThatMightGetAPropagation;
    set<const Function*> functionsIveAlreadySeen;

    auto addCallersToMightPropagateSet = [&](const Function* F) {
        auto callersIt = Ctx->Callers.find(F);
        if (callersIt != Ctx->Callers.end()) {
            for (const auto* call : callersIt->second)
                functionsThatMightGetAPropagation.emplace(call->getFunction());
        }
    };

    for (const auto* associatedErrorHandlerFunction : associatedErrorHandlerFunctions) {
        addCallersToMightPropagateSet(associatedErrorHandlerFunction);
    }

    unsigned int iterationNumber = 1;
    while (!functionsThatMightGetAPropagation.empty()) {
        auto functionThatMightGetAPropagation = functionsThatMightGetAPropagation.top();
        functionsThatMightGetAPropagation.pop();
        if (!functionsIveAlreadySeen.insert(functionThatMightGetAPropagation).second)
            continue;

        FunctionErrorReturnIntervals newIntervals;

        auto functionKeyPair = make_pair(functionThatMightGetAPropagation, 0);

        if (functionThatMightGetAPropagation->getReturnType()->isVoidTy())
            continue;
        if (returnsOnlyOneConstant(functionKeyPair.first))
            continue;

        //LOG(LOG_INFO, "learnErrorsFromErrorBlocksForSelf: " << functionThatMightGetAPropagation->getName() << "\n");

        bool added = false;
        for (const auto& BB : *functionThatMightGetAPropagation) {
            // NOTE: don't check for amount of successors, because of certain fallthrough path constructions
            auto errorHandlingBlock = determineSuccessorOfAbstractComparisonWhichHandlesErrors(&BB);
            if (!errorHandlingBlock) continue;
            vector<const BasicBlock*> blocks;
            DataFlowAnalysis::getLinearUniquePathForwards(errorHandlingBlock, blocks);
            auto returnInstruction = dyn_cast<ReturnInst>(blocks.back()->getTerminator());
            // We don't necessarily have a path slice that terminates in a return (think about slices that are cut short due to other checks).
            if (!returnInstruction) continue;
            auto result = addForSpanAndReturnInstruction(PathSpan{blocks, false}, returnInstruction);
            if (result.has_value()) {
                newIntervals.intervalFor(functionKeyPair, true).unionInPlace(result.value());
                added = true;
            }
        }

        if (added) {
            LOG(LOG_VERBOSE, "Added for: " << functionThatMightGetAPropagation->getName() << " in " << __FUNCTION__ << "\n");
            addCallersToMightPropagateSet(functionThatMightGetAPropagation);
            //newIntervals.dump();
        }

        //newIntervals.dump();
        //LOG(LOG_INFO, "Completed successful iteration number " << iterationNumber << "\n");
        Ctx->functionErrorReturnIntervals.mergeDestructivelyForOther(newIntervals);
        ++iterationNumber;
    }
}

void EHBlockDetectorPass::doModulePass(Module *M) {
    if (stage == 0)
        stage0(M);
    else if (stage == 1)
        stage1(M);
    else
        assert(false);
}
