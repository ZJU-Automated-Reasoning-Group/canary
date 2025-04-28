//===-- DataFlowAnalysis.cc - Utils for data-flow analysis------===//
// 
// This file implements commonly used functions for data-flow
// analysis
//
//===-----------------------------------------------------------===//

#include <llvm/ADT/Optional.h>

#include <llvm/IR/InstIterator.h>
#include <llvm/IR/AbstractCallSite.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Constants.h>
#include <algorithm>

#include "Checker/ESSS/DataFlowAnalysis.h"
#include "Checker/ESSS/Helpers.h"
#include "Checker/ESSS/DebugHelpers.h"
#include "Checker/ESSS/Config.h"
#include <numeric>
#include <set>

//#define DEBUG_PRINT_VALUE_RESOLUTION

extern GlobalContext GlobalCtx;


// Make sure we're using llvm's Optional for these methods
llvm::Optional<int> DataFlowAnalysis::computeRhsFromValue(const Value* rhsValue) {
    if (auto constant = dyn_cast<ConstantInt>(rhsValue)) {
        return static_cast<int>(constant->getSExtValue());
    } else if (isa<ConstantPointerNull>(rhsValue)) {
        return 0;
    } else if (auto expr = dyn_cast<ConstantExpr>(rhsValue)) {
        return computeRhsFromValue(expr->getOperand(0));
    }
    return llvm::None;
}

llvm::Optional<int> DataFlowAnalysis::computeRhs(const AbstractComparison& comparison) {
    auto rhs = computeRhsFromValue(comparison.getRhs());
    if (rhs.hasValue()) {
        return rhs;
    } else {
        if (VerboseLevel >= LOG_VERBOSE) {
            LOG(LOG_VERBOSE, "Can't derive rhs\n");
            comparison.dump();
            comparison.getRhs()->dump();
            if (auto I = dyn_cast<Instruction>(comparison.getRhs())) {
                SourceLocation{I}.dump(LOG_INFO);
                LOG(LOG_INFO, "\n");
            }
        }
    }
    return llvm::None;
}

void AbstractComparison::dump() const {
    lhs->printAsOperand(OP, true);
    LOG(LOG_INFO, ' ' << CmpInst::getPredicateName(predicate) << ' ');
    rhs->printAsOperand(OP, true);
    LOG(LOG_INFO, '\n');
}

void AbstractFallback::dump() const {
    lhs->printAsOperand(OP, true);
    LOG(LOG_INFO, '\n');
}

void DataFlowAnalysis::getLinearUniquePathBackwards(const BasicBlock* currentBlock, vector<const BasicBlock*>& blocks) {
    blocks.push_back(currentBlock);
    while ((currentBlock = currentBlock->getUniquePredecessor())) {
        blocks.push_back(currentBlock);
    }
}

void DataFlowAnalysis::getLinearUniquePathForwards(const BasicBlock* currentBlock, vector<const BasicBlock*>& blocks) {
    SmallPtrSet<const BasicBlock*, 8> seenBlocks;
    blocks.push_back(currentBlock);
    while ((currentBlock = currentBlock->getUniqueSuccessor())) {
        if (!seenBlocks.insert(currentBlock).second)
            return;
        blocks.push_back(currentBlock);
    }
}

template<typename Condition, typename R>
R searchBackFromOriginPoint(const Instruction* originPointFromWhereToLookBack, PathSpan blocks, const Condition& condition, R _default) {
    auto curr = blocks.begin();
    auto end = blocks.end();
    bool arrivedAtOriginBlock = false;

    for (; curr != end; ++curr) {
        const BasicBlock* block = *curr;
        for (auto it = block->rbegin(), e = block->rend(); it != e; ++it) {
            const Instruction* instr = &*it;
            if (!arrivedAtOriginBlock) {
                if (instr == originPointFromWhereToLookBack) {
                    arrivedAtOriginBlock = true;
                }
                continue;
            }

            R value = condition(instr);
            if (value) {
                return value;
            }
        }
    }
    return _default;
}

template<typename Condition, typename R>
void searchBackFromOriginPointFindAll(const Instruction* originPointFromWhereToLookBack, PathSpan blocks, const Condition& condition, set<R>& resultSet) {
    auto curr = blocks.begin();
    auto end = blocks.end();
    bool arrivedAtOriginBlock = false;

    for (; curr != end; ++curr) {
        const BasicBlock* block = *curr;
        for (auto it = block->rbegin(), e = block->rend(); it != e; ++it) {
            const Instruction* instr = &*it;
            if (!arrivedAtOriginBlock) {
                if (instr == originPointFromWhereToLookBack) {
                    arrivedAtOriginBlock = true;
                }
                continue;
            }

            R value = condition(instr);
            if (value) {
                resultSet.insert(value);
            }
        }
    }
}

const Value* DataFlowAnalysis::findUndisputedValueWithoutLeavingCurrentPathResolveLoad(const LoadInst* load, PathSpan blocks, PHISet& phiSet) {
#ifdef DEBUG_PRINT_VALUE_RESOLUTION
    if (load->getParent()->getParent()->getName().equals("mbfl_filt_conv_html_dec")){
    LOG(LOG_INFO, "findUndisputedValueWithoutLeavingCurrentPathResolveLoad\n");
    load->dump();
    SourceLocation{load}.dump(LOG_INFO);
    LOG(LOG_INFO, "\n");}
#endif

    // We have to prioritise stores, even on aliases, because of overwriting.
    // Therefore, first try those and only if that fails follow the load.
    // Steps:
    //   1) Collect aliases of the pointer operand
    set<const Value*> aliases;
    aliases.insert(load->getPointerOperand());
    if (auto pointerOperandAsLoad = dyn_cast<LoadInst>(load->getPointerOperand())) {
        searchBackFromOriginPointFindAll(load, blocks, [pointerOperandAsLoad](const Instruction* candidate) -> const Value* {
            auto candidateLoad = dyn_cast<LoadInst>(candidate);
            if (candidateLoad && candidateLoad->getPointerOperand() == pointerOperandAsLoad->getPointerOperand()) {
                return dyn_cast<Value>(candidateLoad);
            }
            return nullptr;
        }, aliases);
    } else if (auto pointerOperandAsGEP = dyn_cast<GetElementPtrInst>(load->getPointerOperand())) {
        auto module = load->getModule();
        APInt candidateOffset(module->getDataLayout().getPointerSize() * 8, 0, true);
        auto base = pointerOperandAsGEP->stripAndAccumulateConstantOffsets(module->getDataLayout(), candidateOffset, true);
        searchBackFromOriginPointFindAll(load, blocks, [&](const Instruction* candidate) -> const Value* {
            if (auto candidateGEP = dyn_cast<GetElementPtrInst>(candidate)) {
                APInt offset(module->getDataLayout().getPointerSize() * 8, 0, true);
                auto baseForCandidate = candidateGEP->stripAndAccumulateConstantOffsets(module->getDataLayout(), offset, true);
                if (candidateOffset == offset && base != baseForCandidate) {
                    auto aaIt = GlobalCtx.AAPass.find(load->getModule());
                    assert(aaIt != GlobalCtx.AAPass.end());
                    if (aaIt->second->getAAResults().alias(base, baseForCandidate) >= AliasResult::MayAlias)
                        return candidateGEP;
                }
            }

            return nullptr;
        }, aliases);
    }
    //   2) Search back from the origin point to stores
    {
        auto gottenStore = searchBackFromOriginPoint(load, blocks, [&](const Instruction* candidate) -> const StoreInst* {
            //assert(candidate);
            //LOG(LOG_INFO, "Candidate: ");
            //candidate->dump();
            auto store = dyn_cast<StoreInst>(candidate);
            if (store && aliases.find(store->getPointerOperand()) != aliases.end()) {
                return store;
            }
            return nullptr;
        }, (const StoreInst*) nullptr);
        if (gottenStore) {
            /*load->dump();
            gottenStore->dump();
            dumpPathList(blocks);*/
            return findUndisputedValueWithoutLeavingCurrentPath(gottenStore->getValueOperand(), gottenStore, blocks, phiSet);
        }
    }

    // It's possible there's a couple of pointer dereferences we have to follow
    if (auto followedLoad = dyn_cast<LoadInst>(load->getPointerOperand())) {
        return findUndisputedValueWithoutLeavingCurrentPath(load->getPointerOperand() /* note followedLoad! */, followedLoad, blocks, phiSet);
    }

    // If we get here, it likely depends on the path towards this value, bailout
    if (isa<GetElementPtrInst>(load->getPointerOperand())) {
        return load->getPointerOperand();
    }
    return nullptr;
}

static bool isInstructionIsOnPath(const Instruction* instruction, esss::span<const BasicBlock* const> blocks) {
    const BasicBlock* bb = instruction->getParent();
    for (const BasicBlock* block : blocks) {
        if (block == bb) {
            return true;
        }
    }
    return false;
}

void DataFlowAnalysis::collectCalls(const Value* V, set<const Value*>& visited, set<const Value*>& result) {
    if (isa<CallInst>(V)) {
        result.insert(V);
        return;
    }

    if (isa<Constant>(V) || isa<Argument>(V) || isa<AllocaInst>(V) || isa<GetElementPtrInst>(V))
        return;

    if (!visited.insert(V).second)
        return;

    if (auto cast = dyn_cast<CastInst>(V)) {
        collectCalls(cast->getOperand(0), visited, result);
    } else if (auto sext = dyn_cast<SExtInst>(V)) {
        collectCalls(sext->getOperand(0), visited, result);
    } else if (auto zext = dyn_cast<SExtInst>(V)) {
        collectCalls(zext->getOperand(0), visited, result);
    } else if (auto phi = dyn_cast<PHINode>(V)) {
        for (unsigned int i = 0, l = phi->getNumIncomingValues(); i < l; ++i) {
            collectCalls(phi->getIncomingValue(i), visited, result);
        }
    } else if (auto freeze = dyn_cast<FreezeInst>(V)) {
        collectCalls(freeze->getOperand(0), visited, result);
    } else if (auto select = dyn_cast<SelectInst>(V)) {
        collectCalls(select->getTrueValue(), visited, result);
        collectCalls(select->getFalseValue(), visited, result);
    }
}

const Value* DataFlowAnalysis::findUndisputedValueWithoutLeavingCurrentPath(const Value* value, const Instruction* /*originPointFromWhereToLookBack*/, PathSpan blocks, PHISet& phiSet) {
    assert(value);

    if (isa<Constant>(value) || isa<CallInst>(value) || isa<Argument>(value) || isa<AllocaInst>(value) || isa<GetElementPtrInst>(value))
        return value;

#ifdef DEBUG_PRINT_VALUE_RESOLUTION
    LOG(LOG_INFO, "findUndisputedValueWithoutLeavingCurrentPath: ");
    value->dump();
    if (auto I = dyn_cast<Instruction>(value))
        SourceLocation{I}.dump(LOG_INFO);
    LOG(LOG_INFO, "\n");
    dumpPathList(blocks.blocks);
#endif

    if (auto load = dyn_cast<LoadInst>(value)) {
        return findUndisputedValueWithoutLeavingCurrentPathResolveLoad(load, blocks, phiSet);
    } else if (auto extractValue = dyn_cast<ExtractValueInst>(value)) {
        // Caller should handle this
        return extractValue;
    } else if (auto cast = dyn_cast<CastInst>(value)) {
        return findUndisputedValueWithoutLeavingCurrentPath(cast->getOperand(0), cast, blocks, phiSet);
    } else if (auto select = dyn_cast<SelectInst>(value)) {
        auto condition = findUndisputedValueWithoutLeavingCurrentPath(select->getCondition(), select, blocks, phiSet);
        if (auto constantInt = dyn_cast_or_null<ConstantInt>(condition)) {
            return findUndisputedValueWithoutLeavingCurrentPath(constantInt->getZExtValue() != 0 ? select->getTrueValue() : select->getFalseValue(), select, blocks, phiSet);
        }
        // Depends on something we don't support
        return select;
    } else if (auto cmp = dyn_cast<CmpInst>(value)) {
        auto comparand = findUndisputedValueWithoutLeavingCurrentPath(cmp->getOperand(0), cmp, blocks, phiSet);
        auto cmpOp1 = comparand ? findUndisputedValueWithoutLeavingCurrentPath(cmp->getOperand(1), cmp, blocks, phiSet) : nullptr;

        // 1) Find all compares on the path and their evaluation.
        // 2) Find out which of those compares share the same comparand.
        // 3) Check whether the condition is compatible
        if (cmpOp1) {
#if 0
            comparand->dump();
            cmpOp1->dump();
            LOG(LOG_INFO, "blocks:\n");
            for (const BasicBlock* block : blocks) {
                LOG(LOG_INFO, "  " << getBasicBlockName(block));
            }
            LOG(LOG_INFO, "\n");
#endif

            // We need to consider all the blocks just before the cmp
            // So our ending iterator is one before the begin, so that the begin is included.
            // And the starting iterator will be found by looping until we match the basic block.
            auto endIt = blocks.begin();
            --endIt;
            auto it = blocks.end();
            --it; // Let it point to the last block
            while (it != endIt && *it != cmp->getParent())
                --it;
            if (it == endIt) return value;
            // Now it points to the block containing the cmp, we'll need to go one past that
            --it;

            for (; it != endIt; --it) {
                auto *block = *it;
                //LOG(LOG_INFO, "Checking block: " << getBasicBlockName(block) << "\n");
                auto itCopy = it;
                ++itCopy;
                auto *nextBlock = *itCopy;
                if (const BranchInst *br = dyn_cast<BranchInst>(block->getTerminator())) {
                    if (!br->isConditional())
                        continue;

                    auto otherCmp = dyn_cast<CmpInst>(br->getCondition());
                    if (otherCmp && otherCmp != cmp) {
                        //LOG(LOG_INFO, "otherCmp: ");
                        //otherCmp->dump();

                        // It's possible to get into a loop when backtracking along the path that's part of a do-while construct
                        if (!phiSet.insert(otherCmp->getOperand(0)).second) {
                            continue;
                        }

                        auto otherComparand = findUndisputedValueWithoutLeavingCurrentPath(otherCmp->getOperand(0),
                                                                                           otherCmp, blocks, phiSet);
                        if (otherComparand == comparand) {
                            bool matchedOp1 = false;
                            auto otherCmpOp1 = findUndisputedValueWithoutLeavingCurrentPath(otherCmp->getOperand(1), otherCmp, blocks, phiSet);
                            if (otherCmpOp1 == cmpOp1) {
                                matchedOp1 = true;
                                ICmpInst::Predicate truthPredicate;
                                if (br->getSuccessor(0) == nextBlock) {
                                    // Condition was true
                                    truthPredicate = otherCmp->getPredicate();
                                } else {
                                    // Condition was false
                                    truthPredicate = otherCmp->getInversePredicate();
                                }

                                if (ICmpInst::isImpliedTrueByMatchingCmp(truthPredicate, cmp->getPredicate())) {
                                    return ConstantInt::getTrue(Type::getInt1Ty(cmp->getContext()));
                                } else if (ICmpInst::isImpliedFalseByMatchingCmp(truthPredicate, cmp->getPredicate())) {
                                    return ConstantInt::getFalse(Type::getInt1Ty(cmp->getContext()));
                                }
                            }
                            if (!matchedOp1) {
                                LOG(LOG_VERBOSE, "Did not match OP1!\n");
                            }
                        }
                    }
                } else if (const SwitchInst* _switch = dyn_cast<SwitchInst>(block->getTerminator())) {
                    auto resolvedCondition = findUndisputedValueWithoutLeavingCurrentPath(_switch->getCondition(), _switch, blocks, phiSet);
                    if (resolvedCondition == comparand) {
                        for (const auto& _case : _switch->cases()) {
                            if (_case.getCaseSuccessor() == nextBlock) {
                                // We're trying to see if the compare instruction "cmp" is true or false.
                                // We now know that the switch condition X equals the comparand op0 of cmp.
                                // We also know that this case is taken => X == _case.getCaseValue()
                                // We want to know whether comparand op0 <cmp OP> cmpOp1
                                // <=> X <cmp OP> cmpOp1
                                // <=> _case.getCaseValue() <cmp OP> cmpOp1
                                auto cmpOp1Constant = dyn_cast<ConstantInt>(cmpOp1);
                                if (!cmpOp1Constant) continue; // Don't break, other case might have the same successor because of immediate fallthrough

                                bool returnValue;
                                if (cmp->getPredicate() == CmpInst::Predicate::ICMP_EQ) {
                                    returnValue = _case.getCaseValue()->getValue().eq(cmpOp1Constant->getValue());
                                } else if (cmp->getPredicate() == CmpInst::Predicate::ICMP_NE) {
                                    returnValue = _case.getCaseValue()->getValue().ne(cmpOp1Constant->getValue());
                                } else if (cmp->getPredicate() == CmpInst::Predicate::ICMP_SLT) {
                                    returnValue = _case.getCaseValue()->getValue().slt(cmpOp1Constant->getValue());
                                } else if (cmp->getPredicate() == CmpInst::Predicate::ICMP_SGE) {
                                    returnValue = _case.getCaseValue()->getValue().sge(cmpOp1Constant->getValue());
                                } else if (cmp->getPredicate() == CmpInst::Predicate::ICMP_SGT) {
                                    returnValue = _case.getCaseValue()->getValue().sgt(cmpOp1Constant->getValue());
                                } else if (cmp->getPredicate() == CmpInst::Predicate::ICMP_SLE) {
                                    returnValue = _case.getCaseValue()->getValue().sle(cmpOp1Constant->getValue());
                                } else if (cmp->getPredicate() == CmpInst::Predicate::ICMP_ULT) {
                                    returnValue = _case.getCaseValue()->getValue().ult(cmpOp1Constant->getValue());
                                } else if (cmp->getPredicate() == CmpInst::Predicate::ICMP_UGE) {
                                    returnValue = _case.getCaseValue()->getValue().uge(cmpOp1Constant->getValue());
                                } else if (cmp->getPredicate() == CmpInst::Predicate::ICMP_UGT) {
                                    returnValue = _case.getCaseValue()->getValue().ugt(cmpOp1Constant->getValue());
                                } else if (cmp->getPredicate() == CmpInst::Predicate::ICMP_ULE) {
                                    returnValue = _case.getCaseValue()->getValue().ule(cmpOp1Constant->getValue());
                                } else {
                                    LOG(LOG_INFO, "Unsupported predicate: " << cmp->getPredicate() << "\n");
                                    _case.getCaseValue()->dump();
                                    cmpOp1->dump();
                                    assert(false && "todo");
                                }

                                if (returnValue) {
                                    return ConstantInt::getTrue(Type::getInt1Ty(cmp->getContext()));
                                } else {
                                    return ConstantInt::getFalse(Type::getInt1Ty(cmp->getContext()));
                                }
                            }
                        }
                    }
                }
            }
        }

        // Unresolvable with certainty
        return value;
    } else if (auto phi = dyn_cast<PHINode>(value)) {
        if (phiSet.insert(phi).second) {
			// NOTE: very important is that we get the *closest* backwards match, because there can be multiple matches...
			unsigned int bestDistance = -1U;
			const Value* bestValue = nullptr;
			for (unsigned int i = 0, l = phi->getNumIncomingValues(); i < l; ++i) {
				auto incomingBlock = phi->getIncomingBlock(i);
				// Avoid loops
				if (incomingBlock == phi->getParent())
					continue;
				auto it = blocks.end();
				--it;
				auto last = blocks.begin();
				--last;
				unsigned int distance = 0;
				for (; it != last && distance < bestDistance; --it) {
					if (*it == incomingBlock) {
						bestDistance = distance;
						bestValue = phi->getIncomingValue(i);
						break;
					}
					++distance;
				}
			}
            if (bestValue) {
				return findUndisputedValueWithoutLeavingCurrentPath(bestValue, phi, blocks, phiSet);
			}
        }
        return value;
    } else if (auto freeze = dyn_cast<FreezeInst>(value)) {
        return findUndisputedValueWithoutLeavingCurrentPath(freeze->getOperand(0), freeze, blocks, phiSet);
    }

#if 0
    if (VerboseLevel >= LOG_VERBOSE) {
        LOG(LOG_VERBOSE, "Unsupported: ");
        value->dump();
        if (auto I = dyn_cast<Instruction>(value)) {
            LOG(LOG_VERBOSE, "\t");
            SourceLocation{I}.dump(LOG_VERBOSE);
            LOG(LOG_VERBOSE, "\n");
        }
    }
#endif

    return nullptr;
}

void DataFlowAnalysis::getPotentialSanityCheck(const BasicBlock& BB, const std::function<void(const AbstractCondition*, pair<const Value*, unsigned int>)>& callback) {
    //LOG(LOG_INFO, "--- getPotentialSanityCheck ---\n");
    auto handleLhs = [&](const Instruction* lhs) -> pair<const Value*, unsigned int> {
        auto handle = [&](PathSpan blocks) -> pair<const Value*, unsigned int> {
            // It's not always the case that the LHS is on the sliced path, because it might get cut out due to
            // another check.
            if (!isInstructionIsOnPath(lhs, blocks.blocks))
                return make_pair(nullptr, 0);

            // Used to differentiate between multiple possible return values
            unsigned int valueIndex = 0;

            PHISet phiSet;
            auto comparand = findUndisputedValueWithoutLeavingCurrentPath(lhs, lhs, blocks, phiSet);

            // NOTE: this handles extract value
            auto extractValue = dyn_cast_or_null<ExtractValueInst>(comparand);
            if (extractValue && extractValue->getNumIndices() == 1) {
                PHISet extractValuePhiSet;
                comparand = findUndisputedValueWithoutLeavingCurrentPath(extractValue->getAggregateOperand(), extractValue, blocks, extractValuePhiSet);
                valueIndex = extractValue->getIndices()[0];
            }

            bool debug = false; //BB.getParent()->getName().equals("asn1_template_ex_i2d");

            if (comparand && (isa<CallInst>(comparand) || /*isa<GetElementPtrInst>(comparand) || isa<Argument>(comparand) || */isa<ICmpInst>(comparand) /* propagate up */)) {
                if (debug) {
                    LOG(LOG_INFO, "getPotentialSanityCheck success:\n");
                    lhs->dump();
                    SourceLocation{lhs}.dump(LOG_INFO);
                    LOG(LOG_INFO, "\nComparand: ");
                    comparand->dump();
                }

                //LOG(LOG_INFO, "Success\n");
                return make_pair(comparand, valueIndex);
            } else {
                if (debug) {
                    LOG(LOG_INFO, "getPotentialSanityCheck failure:\n");
                    lhs->dump();
                    SourceLocation{lhs}.dump(LOG_INFO);
                    LOG(LOG_INFO, "\nComparand: " << comparand << "  ");
                    if (comparand) comparand->dump();
                    dumpPathList(blocks.blocks);
                }
                //LOG(LOG_INFO, "Failure\n");
                return make_pair(nullptr, 0);
            }
        };

        vector<const BasicBlock*> blocks;
        getLinearUniquePathBackwards(&BB, blocks);
        auto retPair = handle(PathSpan(blocks, true));
        if (retPair.first)
            return retPair;

#if 1
        // Handle triangle construction in CFG
        auto firstBlock = *(blocks.end() - 1);
        if (firstBlock->hasNPredecessors(2)) {
            auto predecessorsIt = pred_begin(firstBlock);
            auto copyIt = predecessorsIt;
            auto pred = (*predecessorsIt)->getUniquePredecessor();
            if (pred && pred == *(++copyIt)) {
                blocks.push_back(*predecessorsIt);
#if 0
                dumpPathList(blocks);
#endif
                return handle(PathSpan(blocks, true));
            } else {
                pred = (*copyIt)->getUniquePredecessor();
                if (pred && pred == *(++predecessorsIt)) {
                    blocks.push_back(*copyIt);
#if 0
                    dumpPathList(blocks);
#endif
                    return handle(PathSpan(blocks, true));
                }
            }
        }
#endif

        return make_pair(nullptr, 0);
    };

    pair<const Value*, unsigned int> value;

    if (auto _switch = dyn_cast<SwitchInst>(BB.getTerminator())) {
        if (auto lhs = dyn_cast<Instruction>(_switch->getCondition())) {
            value = handleLhs(lhs);
            if (value.first) {
                assert(!isa<CmpInst>(value.first));
                for (const auto& _case: _switch->cases()) {
                    callback(new AbstractComparison(ICmpInst::Predicate::ICMP_EQ, value.first, _case.getCaseValue(),
                                                    _switch, true), value);
                }
                callback(new AbstractFallback(_switch->getDefaultDest(), _switch), value);
            }
        }
    } else {
        // TODO: maybe optimize this with case specialisation?
        for (const auto& instruction : BB) {
            ICmpInst* cmp = nullptr;
            bool fromConditionalBranch = false;

            if (isa<ZExtInst>(instruction) || isa<SExtInst>(instruction)) {
                cmp = dyn_cast<ICmpInst>(instruction.getOperand(0));
                fromConditionalBranch = false;
            } else {
                auto br = dyn_cast<BranchInst>(&instruction);
                if (br && br->isConditional()) {
                    cmp = dyn_cast<ICmpInst>(br->getCondition());
                    fromConditionalBranch = true;
                }
            }

            if (cmp) {
                if (auto lhs = dyn_cast<Instruction>(cmp->getOperand(0))) {
                    value = handleLhs(lhs);
                    if (value.first) {
                        if (auto nestedCmp = dyn_cast<ICmpInst>(value.first)) {
                            auto constant = dyn_cast<ConstantInt>(nestedCmp->getOperand(1));
                            if (!constant || !constant->isZero()) {
                                return;
                            }

                            auto nestedCmpI = dyn_cast<Instruction>(nestedCmp->getOperand(0));
                            if (!nestedCmpI) return;
                            auto resolvedNested = handleLhs(nestedCmpI);
                            if (!resolvedNested.first) return;

                            if (cmp->getPredicate() == llvm::CmpInst::ICMP_EQ) {
                                // nestedCmp == 0 <=> !nestedCmp
                                callback(new AbstractComparison(nestedCmp->getInversePredicate(), resolvedNested.first, nestedCmp->getOperand(1), cmp, fromConditionalBranch), resolvedNested);
                            } else {
                                assert(cmp->getPredicate() == llvm::CmpInst::ICMP_NE);
                                // nestedCmp != 0 <=> nestedCmp
                                callback(new AbstractComparison(nestedCmp->getPredicate(), resolvedNested.first, nestedCmp->getOperand(1), cmp, fromConditionalBranch), resolvedNested);
                            }
                        } else {
                            callback(new AbstractComparison(cmp, fromConditionalBranch), value);
                        }
                    }
                }
            }
        }
    }
}
