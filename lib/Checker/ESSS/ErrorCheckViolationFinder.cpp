#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/IntrinsicInst.h>
#include <sys/wait.h>

#include "Checker/ESSS/EHBlockDetector.h"
#include "Checker/ESSS/ErrorCheckViolationFinder.h"
#include "Checker/ESSS/DataFlowAnalysis.h"
#include "Checker/ESSS/Common.h"
#include "Checker/ESSS/Helpers.h"
#include "Checker/ESSS/ClOptForward.h"
#include "Checker/ESSS/Helpers.h"
#include "Checker/ESSS/DebugHelpers.h"

//#define NO_INLINE_COUNTING_INCORRECT
#define NO_INLINE_COUNTING_MISSING
//#define DEBUG_INTERVAL_PROPAGATION
#define REPORT
//#define DEBUG_STORING_REPORT_DATA

bool ErrorCheckViolationFinderPass::doInitialization(Module * /* M */) {
    return false;
}

bool ErrorCheckViolationFinderPass::doFinalization(Module * /* M */) {
    return false;
}

void ErrorCheckViolationFinderPass::finish() {
    // Show learned rules for inspection by user and report bugs.
    Ctx->functionErrorReturnIntervals.dump();
    report();
}

void ErrorCheckViolationFinderPass::determineMissingChecksAndPropagationRules(const Function& function, const FunctionErrorReturnIntervals& inputErrorIntervals, FunctionErrorReturnIntervals& outputErrorIntervals, set<const Function*>& functionsToInspectNext, unordered_set<uintptr_t>& handledFunctionPairs, map<pair<const Function*, unsigned int>, Interval>& replaceMap) {
    LOG(LOG_VERBOSE, "determineMissingChecksAndPropagationRules: " << function.getName() << "\n");

    // 1) collect all call instructions into a set "instSet"
    // 2) collect the compare instructions and use the same DFA as in the rule derivation to find out what the corresponding call is (if any)
    // 2.1) check whether the call is sufficiently checked
    // 2.2) remove the instruction from "instSet"
    // 3) report all cases that are still left in "instSet"

    set<const Value*> instSet;
    set<const ICmpInst*> comparisons;
    for (const auto& instruction : instructions(function)) {
        auto maybeCallees = getCalleeIteratorForPotentialCallInstruction(*Ctx, instruction);
        if (maybeCallees.hasValue()) {
            if (any_of(maybeCallees.getValue()->second, [&](const Function* target) {
                auto maybeInterval = inputErrorIntervals.maybeIntervalFor(make_pair(target, 0));
                return maybeInterval.hasValue() && !maybeInterval.getValue()->empty();
            })) {
                instSet.insert(maybeCallees.getValue()->getFirst());
            }
        }
    }

    // Find cases where a function F returns the result of another function that must be error-checked,
    // If the function F did not check it itself.
    // In those cases, F must be error-checked as well.
    if (!function.getReturnType()->isVoidTy() && any_of(instSet, [](const Value* value) {
        return isa<CallInst>(value);
    })) {
        auto functionPair = make_pair(&function, 0 /* Only return values supported right now */);
        auto oldInterval = inputErrorIntervals.maybeIntervalFor(functionPair);

        // All error values need to be considered.
        // We therefore take the union of the intervals.
        Interval thisFunctionInterval;

        // Find all paths forwards starting from the interesting instructions towards the returns
        // and then check if the call is returned.
        // We first collect all starting BBs and their paths such that we don't do duplicate work.
        set<const BasicBlock*> roots;
        for (const auto* value : instSet) {
            if (auto inst = dyn_cast<Instruction>(value)) {
                roots.insert(inst->getParent());
            }
        }

        set<const Function*> learnedFromSet;

        for (const auto* root : roots) {
            // Go back early enough such that we get as much of this path.
            // If a block has a unique predecessor, then its unique predecessor must be executed if the block is
            // executed, so it makes sense to include those predecessors as well to gain as much information as possible
            // (this relation works transitively too).
            {
                auto previous = root->getUniquePredecessor();
                while (previous && succ_size(previous) == 1) {
                    root = previous;
                    previous = root->getUniquePredecessor();
                }
            }

            // Collect paths starting from this call instruction to the end(s) of the function.
            // We can use collectPaths for this with basicBlocksOfNonInterest == {}
            set<const BasicBlock*> emptySet;
            Path* myCurrentPath = new Path();
            vector<Path*> allPaths;
            allPaths.push_back(myCurrentPath);
            EHBlockDetectorPass::collectPaths(root, allPaths, myCurrentPath, emptySet);

            for (auto* path : allPaths) {
                auto lastBB = path->blocks.back();
                if (auto ret = dyn_cast<ReturnInst>(lastBB->getTerminator())) {
                    auto returnValueIndex = 0; // Only return values supported right now

                    PHISet phiSet;
                    if (auto resolvedValue = DataFlowAnalysis::findUndisputedValueWithoutLeavingCurrentPath(ret->getReturnValue(), ret, PathSpan(path->blocks, false), phiSet)) {
                        instSet.erase(dyn_cast<Instruction>(resolvedValue));
                        // Handle the case where we get a cmp first instead of a call
                        if (auto cmp = dyn_cast<ICmpInst>(resolvedValue)) {
#if 1
                            resolvedValue = DataFlowAnalysis::findUndisputedValueWithoutLeavingCurrentPath(cmp->getOperand(0), cmp, PathSpan(path->blocks, false), phiSet);
                            if (!resolvedValue) continue;
                            auto checkedCall = dyn_cast<CallInst>(resolvedValue);
                            if (!checkedCall) continue;
                            auto rhs = DataFlowAnalysis::computeRhsFromValue(cmp->getOperand(1));
                            if (!rhs.hasValue()) continue;
                            llvm::Optional<bool> trueTakenBranchIsErrorOpt = EHBlockDetectorPass::determineErrorBranchOfCallWithCompare(cmp->getPredicate(), returnValueIndex, rhs.getValue(), checkedCall);
                            if (!trueTakenBranchIsErrorOpt.hasValue()) continue;
                            bool trueTakenBranchIsError = trueTakenBranchIsErrorOpt.getValue();
                            // There is not really a "branch" here, just a value. The "true branch" conceptually returns 1
                            // while the "false branch" returns 0.
                            Interval interval;
                            if (trueTakenBranchIsError) {
                                if (any_of(cmp->users(), [](const Value* user) {
                                    return isa<SExtInst>(user);
                                })) {
                                    interval.appendUnsafeBecauseExpectsSortMaintained(Range(-1, -1));
                                } else {
                                    interval.appendUnsafeBecauseExpectsSortMaintained(Range(1, 1));
                                }
                            } else {
                                interval.appendUnsafeBecauseExpectsSortMaintained(Range(0, 0));
                            }
                            thisFunctionInterval.unionInPlace(interval);
                            //thisFunctionInterval.dump();
                            auto maybeCalleesIt = getCalleeIteratorForPotentialCallInstruction(*Ctx, *checkedCall);
                            if (maybeCalleesIt.hasValue()) {
                                for (const auto* target : maybeCalleesIt.getValue()->second) {
                                    learnedFromSet.insert(target);
                                }
                            }
#endif
                        } else if (auto call = dyn_cast<CallInst>(resolvedValue)) {
                            instSet.erase(dyn_cast<Instruction>(resolvedValue));
                            // The call instruction can have many targets. All error values need to be considered.
                            // We therefore take the union of the intervals.
                            Interval intersected(false);
                            auto maybeCalleesIt = getCalleeIteratorForPotentialCallInstruction(*Ctx, *call);
                            if (maybeCalleesIt.hasValue()) {
                                for (const auto* target : maybeCalleesIt.getValue()->second) {
                                    auto maybeInterval = inputErrorIntervals.maybeIntervalFor(make_pair(target, returnValueIndex));
                                    if (maybeInterval.hasValue() && !maybeInterval.getValue()->empty()) {
                                        intersected.intersectionInPlace(*maybeInterval.getValue());
                                        learnedFromSet.insert(target);
                                    }
                                }
                            }
                            thisFunctionInterval.unionInPlace(intersected);
                            //thisFunctionInterval.dump();
                        }
                    } else {
                        instSet.erase(dyn_cast<Instruction>(ret->getReturnValue()));
                    }
                }
            }

            // Cleanup memory
            for (auto* path : allPaths)
                delete path;
        }

        if (!thisFunctionInterval.empty() && !thisFunctionInterval.full()) {
#ifdef DEBUG_INTERVAL_PROPAGATION
            LOG(LOG_INFO, "Learned new interval by propagation for " << function.getName() << "\n");
            for (const auto* learnedFromFunction : learnedFromSet) {
                LOG(LOG_INFO, "\tFrom: " << learnedFromFunction->getName() << "\n");
                if (auto maybeInterval = inputErrorIntervals.maybeIntervalFor(make_pair(learnedFromFunction, 0))) {
                    LOG(LOG_INFO, "\t  ");
                    maybeInterval.getValue()->dump();
                }
            }
            thisFunctionInterval.dump();
            if (auto maybeInterval = inputErrorIntervals.maybeIntervalFor(make_pair(&function, 0))) {
                LOG(LOG_INFO, "have maybe interval\n");
                maybeInterval.getValue()->dump();
            }
#endif

            // Replace the interval if we have better results for this proposed interval than what we had before.
            // Otherwise, insert it.
            auto getConfidence = [this](const Function* function, bool& notFound) -> float {
                notFound = false;
                auto it = Ctx->functionToConfidence.find(make_pair(function, 0 /* Only return values supported right now */));
                if (it == Ctx->functionToConfidence.end()) {
                    LOG(LOG_INFO, "Not found in totalFunctionToIntervalCounts\n");
                    notFound = true;
                    return 0;
                }
                return it->second;
            };

            if (oldInterval.hasValue()) {
                Interval intersection = oldInterval.getValue()->intersection(thisFunctionInterval);

                bool notFound;
                auto currentSupport = getConfidence(&function, notFound);
                if (notFound) {
                    // NOTE: must come from another analysis than this one or the matching analysis
                    currentSupport = 1.0f;
                }
                bool edit = notFound && learnedFromSet.size() > 1;
                float avgNewConfidence = 0;
#if 0
                if (all_of(function, [](const BasicBlock& BB) -> bool {
                    if (auto ret = dyn_cast<ReturnInst>(BB.getTerminator())) {
                        return isa<CallInst>(ret->getReturnValue());
                    }
                    return true;
                })) {
                    edit = true;
                    sumNewSupport = -1U;
                }
#endif

                if (!edit) {
                    for (const auto *learnedFromFunction: learnedFromSet) {
                        bool garbage;
                        avgNewConfidence += getConfidence(learnedFromFunction, garbage);
                    }
                    avgNewConfidence /= static_cast<float>(learnedFromSet.size());
                    LOG(LOG_VERBOSE,
                        ">> Confidence current: " << format("%.2f", currentSupport * 100.0f) << "%, proposed: " << format("%.2f", avgNewConfidence * 100.0f) << "%\n");
                    edit = currentSupport < avgNewConfidence;
                }

                if (edit) {
                    /*if (intersection.empty() || oldInterval.getValue()->size() > thisFunctionInterval.size()) */{
                        Ctx->functionToConfidence[functionPair] = avgNewConfidence;
                        replaceMap.emplace(functionPair, std::move(thisFunctionInterval));
                    }
                }
            } else {
                outputErrorIntervals.insertIntervalFor(functionPair, std::move(thisFunctionInterval));
            }

            // The functions that must be inspected are the callers.
            auto callersIt = Ctx->Callers.find(&function);
            if (callersIt != Ctx->Callers.end()) {
                for (const auto* caller : callersIt->second) {
                    auto key = reinterpret_cast<uintptr_t>(caller->getFunction()) ^ reinterpret_cast<uintptr_t>(&function);
                    if (handledFunctionPairs.insert(key).second)
                        functionsToInspectNext.insert(caller->getFunction());
                }
            }
        }
    }

    // Remove the instructions from "instSet" which are checked for sure
    auto it = Ctx->functionToSanityValuesAndConditions.find(&function);
    if (it != Ctx->functionToSanityValuesAndConditions.end()) {
        for (const auto &item : it->second) {
            const auto &pair = item.first;
            instSet.erase(pair.first);
        }
    }

    // Report unhandled errors (which are also not propagated)
    if (!instSet.empty()) {
#ifdef REPORT
        LOG(LOG_INFO, "Unhandled error checks:\n");
        set<const Value*> result;
        for (const auto& instruction : instructions(function)) {
            set<const Value*> visited;
            if (isa<ICmpInst>(instruction)) {
                DataFlowAnalysis::collectCalls(instruction.getOperand(0), visited, result);
            } else if (auto br = dyn_cast<BranchInst>(&instruction)) {
                if (br->isConditional()) {
                    DataFlowAnalysis::collectCalls(br->getCondition(), visited, result);
                }
            } else if (auto ret = dyn_cast<ReturnInst>(&instruction)) {
                if (ret->getReturnValue()) {
                    DataFlowAnalysis::collectCalls(ret->getReturnValue(), visited, result);
                }
            }
        }

        for (const auto* value : instSet) {
            if (value->getType()->isPointerTy() && !(any_of(value->users(), [](const Value* user) {
                return isa<GetElementPtrInst>(user);
            }) && all_of(value->users(), [](const Value* user) {
                return isa<GetElementPtrInst>(user) || isa<DbgDeclareInst>(user) || isa<DbgValueInst>(user);
            }))) {
                continue;
            } else if (any_of(value->users(), [](const Value *user) {
                return !isa<DbgDeclareInst>(user) && !isa<DbgValueInst>(user);
            })) {
                continue;
            }

            if (!visited.insert(value).second)
                continue;

            auto call = dyn_cast<CallInst>(value);

#ifdef NO_INLINE_COUNTING_MISSING
            if (call->getDebugLoc().get() && call->getDebugLoc().getInlinedAt()) {
                // The set contains both the instruction and the debug locations.
                // This is because although it may be inlined, the original may not be in the source code anymore.
                // Therefore we can't just unconditionally skip the inlined cases.
                if (!visited.insert(call->getDebugLoc().get()).second)
                    continue;
            }
#endif
            if (result.find(value) != result.end()) {
                if (auto callees = getCalleeIteratorForPotentialCallInstruction(GlobalCtx, *call)) {
                    auto &counts = errorFunctionToCountPairsFor(CountPairType::Missing);
                    for (const auto* callee : callees.getValue()->second) {
                        counts[callee].total++;
                    }
                }
            } else {
                if (auto callees = getCalleeIteratorForPotentialCallInstruction(GlobalCtx, *call)) {
                    incorrectErrorReports[SourceLocation{call, false}].emplace_back(IncorrectCheckErrorReport {
                            .intervals = {},
                            .call = call,
                    });
                    auto &counts = errorFunctionToCountPairsFor(CountPairType::Missing);
                    for (const auto* callee : callees.getValue()->second) {
                        counts[callee].incorrect++;
                        counts[callee].total++;
                    }
                }
            }
        }
#endif
    }
}

void ErrorCheckViolationFinderPass::determineIncorrectChecks(const Function& function) {
#ifndef REPORT
    return;
#endif

    auto it = Ctx->functionToSanityValuesAndConditions.find(&function);
    if (it == Ctx->functionToSanityValuesAndConditions.end()) return;
    auto& pairs = it->second;

    map<pair<const Value*, unsigned int>, pair<Interval, Interval>> valueToInterval;
    for (const auto& item : pairs) {
        const auto &pair = item.first;
        const auto &conditional = item.second;
        
        auto comparison = dyn_cast<AbstractComparison>(conditional);
        if (!comparison) continue;

        auto rhs = DataFlowAnalysis::computeRhs(*comparison);
        if (!rhs.hasValue()) continue;

        // NOTE: we don't know what the error branch is, so we try both possible intervals.
        Interval interval1(false);
        interval1.applyInequalityOperator(comparison->getPredicate(), rhs.getValue());
        auto& intervals = valueToInterval[pair];
        intervals.first.unionInPlace(interval1);
        Interval interval2(false);
        interval2.applyInequalityOperator(comparison->getInversePredicate(), rhs.getValue());
        intervals.second.unionInPlace(interval2);
    }
    for (auto& item : valueToInterval) {
        auto &pair = item.first;
        auto &intervals = item.second;
        
        auto instruction = pair.first;
        auto valueIndex = pair.second;
        auto call = dyn_cast<CallInst>(instruction);
        if (!call) continue;

        // Check if it is sufficiently checked by determining the interval of the comparison,
        // this means that the determined error interval is a non-strict subset of the determined interval here.
        auto check = [&](const Interval& comparisonInterval) {
            auto calleesIt = Ctx->Callees.find(call);
            if (calleesIt != Ctx->Callees.end()) {
                return all_of(calleesIt->second, [&](const Function* target) {
                    auto maybeInterval = Ctx->functionErrorReturnIntervals.maybeIntervalFor(make_pair(target, valueIndex));
                    if (!maybeInterval.hasValue() || maybeInterval.getValue()->empty() || maybeInterval.getValue()->full())
                        return true;
                    return maybeInterval.getValue()->isSubsetOf(comparisonInterval);
                });
            }
            return true; // It's fine
        };

        // NOTE: Either success or error has to be checked, so we check the error interval and its inverse.
        bool isCheckedCorrectly;
        {
            isCheckedCorrectly = check(intervals.first);
        }
        if (!isCheckedCorrectly) {
            isCheckedCorrectly = check(intervals.second);
        }

        if (isCheckedCorrectly) {
            if (auto callees = getCalleeIteratorForPotentialCallInstruction(GlobalCtx, *call)) {
                auto &counts = errorFunctionToCountPairsFor(CountPairType::Incorrect);
                for (const auto* callee : callees.getValue()->second) {
                    counts[callee].total++;
                }
            }
        } else {
#ifdef DEBUG_STORING_REPORT_DATA
            if (auto callees = getCalleeIteratorForPotentialCallInstruction(GlobalCtx, *call)) {
                for (const auto* callee : callees.getValue()->second) {
                    auto maybeInterval = Ctx->functionErrorReturnIntervals.maybeIntervalFor(make_pair(callee, valueIndex));
                    if (maybeInterval.hasValue()) {
                        LOG(LOG_INFO, "->" << callee->getName() << "\n");
                        maybeInterval.getValue()->dump();
                    }
                }
            }
#endif
            incorrectErrorReports[SourceLocation{call, false}].emplace_back(IncorrectCheckErrorReport {
                .intervals = std::move(intervals),
                .call = call,
            });

#ifdef NO_INLINE_COUNTING_INCORRECT
            if (!(call->getDebugLoc().get() && call->getDebugLoc().getInlinedAt())) {
#else
            if(true) {
#endif
                auto &counts = errorFunctionToCountPairsFor(CountPairType::Incorrect);
                if (auto callees = getCalleeIteratorForPotentialCallInstruction(GlobalCtx, *call)) {
                    for (const auto *callee: callees.getValue()->second) {
                        auto &data = counts[callee];
                        data.total++;
                        data.incorrect++;
#ifdef DEBUG_STORING_REPORT_DATA
                        LOG(LOG_INFO, "Total++, Incorrect++ for " << callee->getName() << "\n");
                        call->dump();
#endif
                    }
                }
            }
        }
    }
}

void ErrorCheckViolationFinderPass::report() const {
    for (const auto& pair : incorrectErrorReports) {
        const auto& baseLocation = pair.first;
        const auto& reports = pair.second;
        auto call = reports[0].call;
        //SourceLocation callSourceLocation{call};

        if (!baseLocation.isValidProgramSource())
            continue;

        bool isIncorrectCase = reports[0].intervals.hasValue();

        CountPair sum;
        float amount = 0.0f;
        if (auto callees = getCalleeIteratorForPotentialCallInstruction(GlobalCtx, *call)) {
            // If a function is pure, its result is purely dependent on the input and thus should be handled in a context-sensitive way.
            if (all_of(callees.getValue()->second, isProbablyPure)) {
                continue;
            }

            auto &counts = errorFunctionToCountPairsFor(isIncorrectCase ? CountPairType::Incorrect : CountPairType::Missing);
            for (const auto *callee: callees.getValue()->second) {
                sum = sum + counts.find(callee)->second;
                amount++;
            }
        }

#if 1
        sum.incorrect /= amount;
        sum.total /= amount;
#endif
        float score = wilsonScore(sum.total - sum.incorrect, sum.total, 1.645f /* Z for 90% confidence interval */);

        bool filter = score < (isIncorrectCase ? IncorrectCheckThreshold :MissingCheckThreshold);
        unsigned int logLevel = LOG_INFO;
        if (filter) {
            LOG(LOG_VERBOSE, "Skip: ");
            logLevel = LOG_VERBOSE;
        } else {
            if (isIncorrectCase) {
                LOG(LOG_INFO, "Potential bug, not all error values are checked for the following call: ");
            } else {
                LOG(LOG_INFO, "Potential bug, missing check for the following call: ");
            }
        }
        baseLocation.dump(logLevel);
        LOG(logLevel, '\n');
        if (logLevel <= VerboseLevel) {
            call->dump();
        }
        LOG(logLevel, "  Score: " << format("%.2f", score * 100.0f) << "% (I=" << sum.incorrect / amount << ", C="
                                  << (sum.total - sum.incorrect) / amount << ")\n");
#if 0
        if (auto calledFunction = call->getCalledFunction(); auto subProgram = calledFunction->getSubprogram()) {
            LOG(LOG_INFO, "  Called function defined at: " << subProgram->getFilename() << ": " << subProgram->getLine() << "\n");
        }
#endif
        if (filter) {
            continue;
        }

        bool didOutputCallees = false;
        for (const auto& report : reports) {
            const auto &intervals = report.intervals;
            LOG(LOG_INFO, "  Reported at: ");
            auto callLocation = SourceLocation{report.call};
            callLocation.dump();
            LOG(LOG_INFO, "\n");
            if (auto callees = getCalleeIteratorForPotentialCallInstruction(GlobalCtx, *call)) {
                Interval unionOfIntervals(true);
                Interval intersectionOfIntervals(false);
                for (const auto *callee: callees.getValue()->second) {
                    auto calleeInterval = Ctx->functionErrorReturnIntervals.maybeIntervalFor(make_pair(callee, 0));
                    if (calleeInterval.hasValue() && !calleeInterval.getValue()->empty()) {
                        unionOfIntervals.unionInPlace(*calleeInterval.getValue());
                        intersectionOfIntervals.intersectionInPlace(*calleeInterval.getValue());
                    }
                }
                if (!didOutputCallees) {
                    LOG(LOG_INFO, "  Callees:\n");
                    for (const auto *callee: callees.getValue()->second) {
                        auto calleeInterval = Ctx->functionErrorReturnIntervals.maybeIntervalFor(make_pair(callee, 0));
                        if (calleeInterval.hasValue() && !calleeInterval.getValue()->empty()) {
                            LOG(LOG_INFO, "    -> " << callee->getName() << "\n");
                            LOG(LOG_INFO, "       ");
                            calleeInterval.getValue()->dump();
                        }
                    }
                    didOutputCallees = true;
                }
                LOG(LOG_INFO, "  Union of error values: ");
                unionOfIntervals.dump();
                if (intervals.hasValue()) {
                    LOG(LOG_INFO, "  Checked intervals:\n    * ");
                    intervals.getValue().first.dump();
                    LOG(LOG_INFO, "    * ");
                    intervals.getValue().second.dump();
                }
                if (intersectionOfIntervals.empty()) {
                    LOG(LOG_INFO, "  Note: it is odd that the intersection of the error intervals for the callees is empty\n");
                }
            }
        }
    }
}

void ErrorCheckViolationFinderPass::performReplaces(map<pair<const Function*, unsigned int>, Interval>& replaceMap) {
    for (auto& mapEntry : replaceMap) {
        const auto& pair = mapEntry.first;
        auto& interval = mapEntry.second;
        Ctx->functionErrorReturnIntervals.replaceIntervalFor(pair, std::move(interval));
    }
}

void ErrorCheckViolationFinderPass::doModulePass(llvm::Module* M) {
    if (stage == 0)
        stage0(M);
    else
        stage1(M);
}

void ErrorCheckViolationFinderPass::stage0(Module *M) {
    {
        unordered_set<uintptr_t> handledFunctionPairs;
        FunctionErrorReturnIntervals outputErrorIntervals;
        set<const Function *> functionsToInspectNext;
        map<pair<const Function*, unsigned int>, Interval> replaceMap;
        for (const auto &function: *M) {
            if (!Ctx->shouldSkipFunction(&function))
                determineMissingChecksAndPropagationRules(function, Ctx->functionErrorReturnIntervals,
                                                          outputErrorIntervals, functionsToInspectNext, handledFunctionPairs, replaceMap);
        }
        {
            performReplaces(replaceMap);
            Ctx->functionErrorReturnIntervals.mergeDestructivelyForOther(outputErrorIntervals);
        }
        bool ran = false;
        while (!outputErrorIntervals.empty() && !functionsToInspectNext.empty()) {
            ran = true;
            //LOG(LOG_INFO, "Iteration number: " << iterationNumber << "\n");

            // Inspect error-propagation receiving functions.
            LOG(LOG_VERBOSE, "Inspecting " << functionsToInspectNext.size() << " functions\n");
            FunctionErrorReturnIntervals outputErrorIntervalsInNext;
            set<const Function *> functionsToInspectNextInNext;
            replaceMap.clear();
            for (const auto *functionToInspectNext: functionsToInspectNext) {
                determineMissingChecksAndPropagationRules(*functionToInspectNext, Ctx->functionErrorReturnIntervals,
                                                          outputErrorIntervalsInNext, functionsToInspectNextInNext, handledFunctionPairs, replaceMap);
            }

            {
                performReplaces(replaceMap);
                // Merge all interval rules together because we need the combined info for detecting incorrect checks!
                Ctx->functionErrorReturnIntervals.mergeDestructivelyForOther(outputErrorIntervals);
            }

            // Prepare next iteration.
            functionsToInspectNext = std::move(functionsToInspectNextInNext);
            outputErrorIntervals = std::move(outputErrorIntervalsInNext);

            // Special merge case for last iteration
            if (!outputErrorIntervals.empty() && functionsToInspectNext.empty()) {
                Ctx->functionErrorReturnIntervals.mergeDestructivelyForOther(outputErrorIntervals);
            }
        }
        if (!ran) {
            Ctx->functionErrorReturnIntervals.mergeDestructivelyForOther(outputErrorIntervals);
        }
    }
}

void ErrorCheckViolationFinderPass::determineTruncationBugs() const {
    for (const auto& entry : Ctx->functionErrorReturnIntervals) {
        const auto& functionKeyPair = entry.first;
        const auto& interval = entry.second;
        if (interval.empty())
            continue;
        auto function = functionKeyPair.first;
        auto callersIt = Ctx->Callers.find(function);
        if (callersIt != Ctx->Callers.end()) {
            for (const auto* callerInst : callersIt->second) {
                if (any_of(callerInst->users(), [](const auto* user) {
                    return isa<ICmpInst>(user) || isa<SwitchInst>(user);
                })) {
                    continue;
                }
                for (const auto* user : callerInst->users()) {
                    if (auto trunc = dyn_cast<TruncInst>(user)) {
                        auto newWidth = trunc->getDestTy()->getIntegerBitWidth();
                        if (newWidth >= 64)
                            continue;

                        int64_t lower = -(static_cast<int64_t>(1) << (newWidth - 1));
                        int64_t upper = (static_cast<int64_t>(1) << (newWidth - 1)) - 1;

                        if (interval.lowest() < lower || interval.highest() > upper) {
                            LOG(LOG_INFO, "Potential bug, truncation of error values: " << callerInst->getFunction()->getName() << " -> " << function->getName() << "\n");
                            user->dump();
                        }
                    }
                }
            }
        }
    }
}

void ErrorCheckViolationFinderPass::determineSignednessBugs() const {
    struct SignednessCounter {
        unsigned int neg1 = 0, zero = 0, pos1 = 0;

        void update(int signedness) {
            if (signedness == -1) neg1++;
            if (signedness == 1) pos1++;
            if (signedness == 0) zero++;
        }

        int verdict() const {
            if (neg1 > zero && neg1 > pos1)
                return -1;
            if (pos1 > zero && pos1 > neg1)
                return 1;
            return 0;
        }
    };
    DenseMap<StringRef, SignednessCounter> typeToSignednessCounter;
    for (const auto& entry : Ctx->functionErrorReturnIntervals) {
        const auto& functionKeyPair = entry.first;
        const auto& interval = entry.second;
        if (interval.empty() || functionKeyPair.first->empty())
            continue;
        int signedness = interval.signedness();
        if (auto calleeSubProgram = functionKeyPair.first->getSubprogram()) {
            if (!calleeSubProgram->getType())
                continue;
            auto calleeTypeArray = calleeSubProgram->getType()->getTypeArray();
            if (calleeTypeArray.size() == 0)
                continue;
            auto returnTypeCallee = dyn_cast_or_null<DIDerivedType>(calleeTypeArray[0]);
            if (!returnTypeCallee)
                continue;
            typeToSignednessCounter[returnTypeCallee->getName()].update(signedness);
        }
    }
    if (VerboseLevel >= LOG_VERBOSE) {
        for (const auto &item : typeToSignednessCounter) {
            const auto &name = item.first;
            const auto &counter = item.second;
            LOG(LOG_VERBOSE, "Signedness " << name << ": " << counter.verdict() << "\t" << counter.neg1 << ", " << counter.zero << "," << counter.pos1 << "\n");
        }
    }
    for (const auto& entry : Ctx->functionErrorReturnIntervals) {
        const auto& functionKeyPair = entry.first;
        const auto& interval = entry.second;
        if (interval.empty() || functionKeyPair.first->empty())
            continue;
        int signedness = interval.signedness();
        if (signedness == 0)
            continue;
        if (auto calleeSubProgram = functionKeyPair.first->getSubprogram()) {
            if (!calleeSubProgram->getType())
                continue;
            auto calleeTypeArray = calleeSubProgram->getType()->getTypeArray();
            if (calleeTypeArray.size() == 0)
                continue;
            auto returnTypeCallee = dyn_cast_or_null<DIDerivedType>(calleeTypeArray[0]);
            if (!returnTypeCallee)
                continue;
            auto verdict = typeToSignednessCounter[returnTypeCallee->getName()].verdict();
            if (verdict != 0 && signedness != verdict) {
                LOG(LOG_INFO, "Potential bug, signedness bug: " << functionKeyPair.first->getName() << "\n");
            }
        }
    }
}

void ErrorCheckViolationFinderPass::stage1(Module* M) {
    for (const auto& function : *M) {
        if (Ctx->shouldSkipFunction(&function))
            continue;

        determineIncorrectChecks(function);
    }
}
