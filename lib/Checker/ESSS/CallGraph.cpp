//===-- CallGraph.cc - Build global call-graph------------------===//
// 
// This pass builds a global call-graph. The targets of an indirect
// call are identified based on two-layer type-analysis.
//
// First layer: matching function type
// Second layer: matching struct type
//
// In addition, loops are unrolled as "if" statements
//
//===-----------------------------------------------------------===//

#include <llvm/Pass.h>
#include <llvm/IR/Instructions.h>
#include "llvm/IR/Instruction.h"
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"  
#include "llvm/IR/InstrTypes.h" 
#include "llvm/IR/BasicBlock.h" 
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include <map>
#include <stack>
#include <vector> 
#include "llvm/IR/CFG.h" 
#include "llvm/Transforms/Utils/BasicBlockUtils.h" 
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Metadata.h"

#include "Checker/ESSS/CallGraph.h"
#include "Checker/ESSS/Common.h"
#include "Checker/ESSS/Helpers.h"
#include "Checker/ESSS/ClOptForward.h"

//#define UNROLL_LOOP_ONCE
#define CONSERVATIVE_PTR_TYPES

DenseMap<size_t, FuncSet> CallGraphPass::typeFuncsMap;
unordered_map<size_t, unordered_set<size_t>> CallGraphPass::typeConfineMap;
unordered_map<size_t, unordered_set<size_t>> CallGraphPass::typeTransitMap;
unordered_set<size_t> CallGraphPass::typeEscapeSet;
const DataLayout *CurrentLayout;

// Find targets of indirect calls based on type analysis: as long as
// the number and type of parameters of a function matches with the
// ones of the callsite, we say the function is a possible target of
// this call.
void CallGraphPass::findCalleesWithType(CallInst *CI, FuncSet &S) {

	if (CI->isInlineAsm())
		return;

	//
	// TODO: performance improvement: cache results for types
	//
	for (Function *F : AddressTakenFuncs) {

		// VarArg
		if (F->getFunctionType()->isVarArg()) {
			// Compare only known args in VarArg.
		}
		// otherwise, the numbers of args should be equal.
		else if (F->arg_size() != CI->arg_size()) {
			continue;
		}

		if (F->isIntrinsic()) {
			continue;
		}

		// Type matching on args.
		bool Matched = true;
		User::op_iterator AI = CI->arg_begin();
		for (Function::arg_iterator FI = F->arg_begin(), 
				FE = F->arg_end();
				FI != FE; ++FI, ++AI) {
			// Check type mis-matches.
			// Get defined type on callee side.
			Type *DefinedTy = FI->getType();
			// Get actual type on caller side.
			Type *ActualTy = (*AI)->getType();

			if (DefinedTy == ActualTy)
				continue;

			// FIXME: this is a tricky solution for disjoint
			// types in different modules. A more reliable
			// solution is required to evaluate the equality
			// of two types from two different modules.
			// Since each module has its own type table, same
			// types are duplicated in different modules. This
			// makes the equality evaluation of two types from
			// two modules very hard, which is actually done
			// at link time by the linker.
			while (DefinedTy->isPointerTy() && ActualTy->isPointerTy()) {
				DefinedTy = DefinedTy->getPointerElementType();
				ActualTy = ActualTy->getPointerElementType();
			}
			if (DefinedTy->isStructTy() && ActualTy->isStructTy() &&
					(DefinedTy->getStructName().equals(ActualTy->getStructName())))
				continue;
			if (DefinedTy->isIntegerTy() && ActualTy->isIntegerTy() &&
					DefinedTy->getIntegerBitWidth() == ActualTy->getIntegerBitWidth())
				continue;
			// TODO: more types to be supported.

#ifdef CONSERVATIVE_PTR_TYPES
			// Make the type analysis conservative: assume universal
			// pointers, i.e., "void *" and "char *", are equivalent to 
			// any pointer type and integer type.
			if (
					(DefinedTy == Int8PtrTy &&
					 (ActualTy->isPointerTy() || ActualTy == IntPtrTy)) 
					||
					(ActualTy == Int8PtrTy &&
					 (DefinedTy->isPointerTy() || DefinedTy == IntPtrTy))
			   )
				continue;
			else {
				Matched = false;
				break;
			}
#else
            Matched = false;
            break;
#endif
		}

		if (Matched)
			S.insert(F);
	}
}


void CallGraphPass::unrollLoops(Function *F) {

	if (F->isDeclaration())
		return;

	DominatorTree DT = DominatorTree();
	DT.recalculate(*F);
	LoopInfo *LI = new LoopInfo();
	LI->releaseMemory();
	LI->analyze(DT);

	// Collect all loops in the function
	set<Loop *> LPSet;
	for (LoopInfo::iterator i = LI->begin(), e = LI->end(); i!=e; ++i) {

		Loop *LP = *i;
		LPSet.insert(LP);

		list<Loop *> LPL;

		LPL.push_back(LP);
		while (!LPL.empty()) {
			LP = LPL.front();
			LPL.pop_front();
			vector<Loop *> SubLPs = LP->getSubLoops();
			for (auto SubLP : SubLPs) {
				LPSet.insert(SubLP);
				LPL.push_back(SubLP);
			}
		}
	}

	for (Loop *LP : LPSet) {

		// Get the header,latch block, exiting block of every loop
		BasicBlock *HeaderB = LP->getHeader();
		SmallVector<BasicBlock *, 4> LatchBS;

		LP->getLoopLatches(LatchBS);

		for (BasicBlock *LatchB : LatchBS) {
			if (!HeaderB || !LatchB) {
				OP<<"ERROR: Cannot find Header Block or Latch Block\n";
				continue;
			}
			// Two cases:
			// 1. Latch Block has only one successor:
			// 	for loop or while loop;
			// 	In this case: set the Successor of Latch Block to the 
			//	successor block (out of loop one) of Header block
			// 2. Latch Block has two successor: 
			// do-while loop:
			// In this case: set the Successor of Latch Block to the
			//  another successor block of Latch block 

			// get the last instruction in the Latch block
			Instruction *TI = LatchB->getTerminator();
			// Case 1:
			if (LatchB->getSingleSuccessor() != nullptr) {
				for (succ_iterator sit = succ_begin(HeaderB); 
						sit != succ_end(HeaderB); ++sit) {  

					BasicBlock *SuccB = *sit;	
					BasicBlockEdge BBE = BasicBlockEdge(HeaderB, SuccB);
					// Header block has two successor,
					// one edge dominate Latch block;
					// another does not.
					if (DT.dominates(BBE, LatchB))
						continue;
					else {
						TI->setSuccessor(0, SuccB);
						//LOG(LOG_INFO, "Unrolled loop in " << SuccB->getParent()->getName() << "\n");
					}
				}
			}
			// Case 2:
			else {
				for (succ_iterator sit = succ_begin(LatchB); 
						sit != succ_end(LatchB); ++sit) {

					BasicBlock *SuccB = *sit;
					// There will be two successor blocks, one is header
					// we need successor to be another
					if (SuccB == HeaderB)
						continue;
					else{
						TI->setSuccessor(0, SuccB);
						//LOG(LOG_INFO, "Unrolled loop in " << SuccB->getParent()->getName() << "\n");
					}
				}	
			}
		}
	}
}

bool CallGraphPass::typeConfineInInitializer(User* Ini) {
	stack<User*> LU;
	LU.emplace(Ini);

	while (!LU.empty()) {
        auto U = LU.top();
		LU.pop();

		for (auto oi = U->op_begin(), oe = U->op_end();
				oi != oe; ++oi) {
			Value *O = *oi;
			Type *OTy = O->getType();
			// Case 1: function address is assigned to a type
			if (auto F = dyn_cast<Function>(O)) {
				Type *ITy = U->getType();
				// TODO: use offset?
				unsigned ONo = oi->getOperandNo();
				typeFuncsMap[typeIdxHash(ITy, ONo)].insert(F);
			}
			// Case 2: a composite-type object (value) is assigned to a
			// field of another composite-type object
			else if (isCompositeType(OTy)) {
				// confine composite types
				Type *ITy = U->getType();
				unsigned ONo = oi->getOperandNo();
				typeConfineMap[typeIdxHash(ITy, ONo)].insert(typeHash(OTy));

				// recognize nested composite types
				LU.emplace(dyn_cast<User>(O));
			}
			// Case 3: a reference (i.e., pointer) of a composite-type
			// object is assigned to a field of another composite-type
			// object
			else if (auto POTy = dyn_cast<PointerType>(OTy)) {
				if (isa<ConstantPointerNull>(O))
					continue;
				// if the pointer points a composite type, skip it as
				// there should be another initializer for it, which
				// will be captured

				// now consider if it is a bitcast from a function
				// address
				if (auto CO = dyn_cast<BitCastOperator>(O)) {
					// TODO: ? to test if all address-taken functions
					// are captured
				}
			}
		}
	}

	return true;
}

bool CallGraphPass::typeConfineInStore(StoreInst *SI) {

	Value *PO = SI->getPointerOperand();
	Value *VO = SI->getValueOperand();

	// Case 1: The value operand is a function
	if (auto F = dyn_cast<Function>(VO)) {
		Type *STy;
		int Idx;
		if (nextLayerBaseType(PO, STy, Idx, DL)) {
			typeFuncsMap[typeIdxHash(STy, Idx)].insert(F);
			return true;
		}
		else {
			// TODO: OK, for now, let's only consider composite type;
			// skip for other cases
			return false;
		}
	}

	// Cast 2: value-based store
	// A composite-type object is stored
	Type *EPTy = dyn_cast<PointerType>(PO->getType())->getElementType();
	Type *VTy = VO->getType();
	if (isCompositeType(VTy)) {
		if (isCompositeType(EPTy)) {
			typeConfineMap[typeHash(EPTy)].insert(typeHash(VTy));
			return true;
		}
		else {
			escapeType(EPTy, SI);
			return false;
		}
	}

	// Case 3: reference (i.e., pointer)-based store
	if (isa<ConstantPointerNull>(VO))
		return false;
	// FIXME: Get the correct types
	PointerType *PVTy = dyn_cast<PointerType>(VO->getType());
	if (!PVTy)
		return false;

	Type *EVTy = PVTy->getElementType();
    auto containingFunction = SI->getFunction();

	// Store something to a field of a composite-type object
	Type *STy;
	int Idx;
	if (nextLayerBaseType(PO, STy, Idx, DL)) {
		// The value operand is a pointer to a composite-type object
		if (isCompositeType(EVTy)) {
			typeConfineMap[typeIdxHash(STy,
					Idx)].insert(typeHash(EVTy)); 
			return true;
        }
        // Example: mm/mempool.c +188: pool->free = free_fn;
        // free_fn is a function pointer from a function
        // argument
		else if (auto argumentValue = dyn_cast<Argument>(VO)) {
            if (canBeUsedInAnIndirectCall(*containingFunction)) {
                // Cannot determine safely which arguments are passed through without running into dependency issues
                //LOG(LOG_VERBOSE, "Cannot determine for: " << containingFunction->getName() << "\n");
                escapeType(STy, SI, Idx);
            } else {
                // No indirect calls to this function. As we have all the direct callers already we can check
                // if all arguments are statically known.
                for (const auto *caller: Ctx->Callers[containingFunction]) {
                    auto callerArgumentValue = caller->getArgOperand(argumentValue->getArgNo());
                    if (auto calledFunction = dyn_cast<Function>(callerArgumentValue)) {
                        typeFuncsMap[typeIdxHash(STy, Idx)].insert(calledFunction);
                    } else {
                        typeFuncsMap[typeIdxHash(STy, Idx)].clear();
                        escapeType(STy, SI, Idx);
                        break;
                    }
                }
            }
		} else {
            //LOG(LOG_VERBOSE, "No argument passing for: " << containingFunction->getName() << "\n");
#if 0
            SourceLocation{SI}.dump(LOG_INFO);
            LOG(LOG_INFO, "\n");
#endif
            SmallPtrSet<const Function*, 1> functions;
            set<const Value*> visited;
            if (collectFunctions(VO, functions, visited)) {
                for (const auto* F : functions)
                    typeFuncsMap[typeIdxHash(STy, Idx)].insert(const_cast<Function*>(F));
            } else {
                escapeType(STy, SI, Idx);
            }
        }
	}

	return false;
}

bool CallGraphPass::typeConfineInCast(CastInst *CastI) {

	// If a function address is ever cast to another type and stored
	// to a composite type, the escaping analysis will capture the
	// composite type and discard it

	Value *ToV = CastI, *FromV = CastI->getOperand(0);
	Type *ToTy = ToV->getType(), *FromTy = FromV->getType();
	if (isCompositeType(FromTy)) {
		transitType(ToTy, FromTy);
		return true;
	}

	if (!FromTy->isPointerTy() || !ToTy->isPointerTy())
		return false;
	Type *EToTy = dyn_cast<PointerType>(ToTy)->getElementType();
	Type *EFromTy = dyn_cast<PointerType>(FromTy)->getElementType();
	if (isCompositeType(EToTy) && isCompositeType(EFromTy)) {
		transitType(EToTy, EFromTy);
		return true;
	}

	return false;
}

void CallGraphPass::escapeType(Type *Ty, const StoreInst* SI, int Idx) {
	if (Idx == -1)
		typeEscapeSet.insert(typeHash(Ty));
	else
		typeEscapeSet.insert(typeIdxHash(Ty, Idx));
}

void CallGraphPass::transitType(Type *ToTy, Type *FromTy,
		int ToIdx, int FromIdx) {
	if (ToIdx != -1 && FromIdx != -1)
		typeTransitMap[typeIdxHash(ToTy, 
				ToIdx)].insert(typeIdxHash(FromTy, FromIdx));
	else
		typeTransitMap[typeHash(ToTy)].insert(typeHash(FromTy));
}

void CallGraphPass::funcSetIntersection(FuncSet &FS1, FuncSet &FS2, 
		FuncSet &FS) {
	FS.clear();
	for (auto F : FS1) {
		if (FS2.find(F) != FS2.end())
			FS.insert(F);
	}
}

// Get the composite type of the lower layer. Layers are split by
// memory loads
Value *CallGraphPass:: nextLayerBaseType(Value *V, Type * &BTy, 
		int &Idx, const DataLayout *DL) {

	// Two ways to get the next layer type: GEPOperator and LoadInst
	// Case 1: GEPOperator
	// Change by void0red: https://github.com/umnsec/crix/pull/7
	if (auto GEP = dyn_cast<GEPOperator>(V)) {
		Type *PTy = GEP->getPointerOperand()->getType();
		Type *Ty = PTy->getPointerElementType();
		if ((Ty->isStructTy() || Ty->isArrayTy() || Ty->isVectorTy()) 
				&& GEP->hasAllConstantIndices()) {
			BTy = Ty;
			User::op_iterator ie = GEP->idx_end();
			ConstantInt *ConstI = dyn_cast<ConstantInt>((--ie)->get());
			Idx = ConstI->getSExtValue();
			return GEP->getPointerOperand();
		}
		else
			return nullptr;
	}
	// Case 2: LoadInst
	else if (LoadInst *LI = dyn_cast<LoadInst>(V)) {
		return nextLayerBaseType(LI->getOperand(0), BTy, Idx, DL);
	}
	// Other instructions such as CastInst
	// FIXME: may introduce false positives
#if 1
	else if (auto UI = dyn_cast<UnaryInstruction>(V)) {
		return nextLayerBaseType(UI->getOperand(0), BTy, Idx, DL);
	}
#endif
	else
		return nullptr;
}

bool CallGraphPass::findCalleesWithMLTA(CallInst *CI, FuncSet &FS) {
	// Initial set: first-layer results
	FuncSet FS1 = Ctx->sigFuncsMap[callHash(CI)];
	if (FS1.empty()) {
		// No need to go through MLTA if the first layer is empty
		return false;
	}

	FuncSet FS2, FST;

	Type *LayerTy = nullptr;
	int FieldIdx = -1;
	Value *CV = CI->getCalledOperand();

	// Get the second-layer type
#ifndef ONE_LAYER_MLTA
	CV = nextLayerBaseType(CV, LayerTy, FieldIdx, DL);
#else
	CV = nullptr;
#endif

    // Filter using debug info on first occurrence of second-layer type
    auto structType = dyn_cast_or_null<StructType>(LayerTy);
    if (structType && static_cast<int>(structType->getNumElements()) > FieldIdx) {
        auto x = CurrentLayout->getStructLayout(structType)->getElementOffsetInBits(FieldIdx);
        //LOG(LOG_INFO, "X: " << x << ", " << structType->getName() << "\n");
        auto typeAndOffsetToDebugTag = typeAndOffsetToDebugTags.find(make_pair(structType->getName().substr(7), x));
        if (typeAndOffsetToDebugTag != typeAndOffsetToDebugTags.end()) {
            //LOG(LOG_INFO, " -> " << typeAndOffsetToDebugTag->second << "\n");
            // Remove the functions which do not match the debug tag
            FuncSet filtered;
            for (auto* f : FS1) {
                auto subProgram = f->getSubprogram();
                if (subProgram) {
                    auto targetDebugTag = computeDebugTag(subProgram->getType());
                    //LOG(LOG_INFO, "  debug tag of " << f->getName() << " -> " << targetDebugTag << "\n");
                    if (targetDebugTag == typeAndOffsetToDebugTag->second) {
                        filtered.insert(f);
                    }
                } else {
                    filtered.insert(f);
                }
            }
            FS1 = std::move(filtered);
        }
    }

	int LayerNo = 1;
	while (CV) {
		// Step 1: ensure the type hasn't escaped
#if 1
		if ((typeEscapeSet.find(typeHash(LayerTy)) != typeEscapeSet.end()) || 
				(typeEscapeSet.find(typeIdxHash(LayerTy, FieldIdx)) !=
				 typeEscapeSet.end())) {
			break;
		}
#endif

		// Step 2: get the funcset and merge
		FS2 = typeFuncsMap[typeIdxHash(LayerTy, FieldIdx)];
		funcSetIntersection(FS1, FS2, FST);

		// Step 3: get transitted funcsets and merge
		// NOTE: this nested loop can be slow
#if 1
		size_t TH = typeHash(LayerTy);
		stack<size_t> LT;
		LT.emplace(TH);
		while (!LT.empty()) {
			auto CT = LT.top();
			LT.pop();

			for (auto H : typeTransitMap[CT]) {
				FS2 = typeFuncsMap[hashIdxHash(H, FieldIdx)];
				funcSetIntersection(FS1, FS2, FST);
				if (!FST.empty())
					FS1 = FST;
			}
		}
#endif

		// Step 4: go to a lower layer
		CV = nextLayerBaseType(CV, LayerTy, FieldIdx, DL);
        if (!FST.empty())
			FS1 = FST;
        ++LayerNo;
	}

	FS = FS1;

#if 0
	if (LayerNo > 1 && FS.size()) {
		OP<<"[CallGraph] Indirect call: "<<*CI<<"\n";
		printSourceCodeInfo(CI);
		OP<<"\n\t Indirect-call targets:\n";
		for (auto F : FS) {
			printSourceCodeInfo(F);
		}
		OP<<"\n";
	}
#endif
	return true;
}

static bool couldPotentiallyBeUsedAsAnIndirectCallTarget(const Function& function) {
    // Fast escape path
    if (!function.hasAddressTaken(nullptr, true, true, true, true))
        return false;

    auto isSafe = [](const User* user, const Value* calledOperand) {
        if (auto call = dyn_cast<CallInst>(user)) {
            if (call->getCalledOperand() == calledOperand)
                return true;
        } else if (auto global = dyn_cast<GlobalValue>(user)) {
            if (!global->getType()->getPointerElementType()->isStructTy())
                return true;
        }
        return false;
    };

    return !all_of(function.users(), [&](const User* user) {
        return isSafe(user, &function) || all_of(user->users(), [&](const User* userUser) {
            return isSafe(userUser, user);
        });
    });
}

bool CallGraphPass::collectFunctions(const Value* value, SmallPtrSet<const Function*, 1>& out, set<const Value*>& visited) const {
    if (!visited.insert(value).second)
        return true;

    if (auto F = dyn_cast<Function>(value)) {
        out.insert(F);
        return true;
    } else if (auto bitcast = dyn_cast<BitCastOperator>(value)) {
        return collectFunctions(bitcast->getOperand(0), out, visited);
    } else if (auto phi = dyn_cast<PHINode>(value)) {
        for (unsigned int i = 0, l = phi->getNumIncomingValues(); i < l; ++i) {
            if (!collectFunctions(phi->getIncomingValue(i), out, visited))
                return false;
        }
        return true;
    }

    return false;
}

bool CallGraphPass::doInitialization(Module *M) {
    CurrentLayout = &M->getDataLayout();
	DL = &M->getDataLayout();
	Int8PtrTy = Type::getInt8PtrTy(M->getContext());
	IntPtrTy = DL->getIntPtrType(M->getContext());

	//
	// Iterate and process globals
	//
	for (Module::global_iterator gi = M->global_begin();
			gi != M->global_end(); ++gi) {
		GlobalVariable* GV = &*gi;
		if (!GV->hasInitializer())
			continue;
		Constant *Ini = GV->getInitializer();
		typeConfineInInitializer(Ini);
	}

    for (auto& alias : M->aliases()) {
        if (auto function = dyn_cast<Function>(alias.getAliasee())) {
            // Link
            size_t fh = funcHash(function, alias.getName());
            if (Ctx->UnifiedFuncMap.find(fh) == Ctx->UnifiedFuncMap.end()) {
                Ctx->UnifiedFuncMap[fh] = function;
                Ctx->UnifiedFuncSet.insert(function);
            }
            Ctx->GlobalFuncs[alias.getName()] = function;
        }
    }

    for (Function &F : *M) {
        if (F.isDeclaration())
            continue;

        // Collect global function definitions.
        if (F.hasExternalLinkage() && !F.empty()) {
            // External linkage always ends up with the function name.
            StringRef FName = F.getName();

            // Map functions to their names.
            Ctx->GlobalFuncs[FName] = &F;
        }

        // Keep a single copy for same functions (inline functions)
        size_t fh = funcHash(&F);
        if (Ctx->UnifiedFuncMap.find(fh) == Ctx->UnifiedFuncMap.end()) {
            Ctx->UnifiedFuncMap[fh] = &F;
            Ctx->UnifiedFuncSet.insert(&F);
        }

        // Collect address-taken functions.
        if (couldPotentiallyBeUsedAsAnIndirectCallTarget(F)) {
            //LOG(LOG_INFO, "Address-taken function that may be an indirect call target: " << F.getName() << "\n");

            auto func = Ctx->UnifiedFuncMap[fh];
            AddressTakenFuncs.insert(func);
            Ctx->sigFuncsMap[funcHash(func, false)].insert(func);
        }
    }

	// Iterate functions and instructions for indirect cases
    if (MLTA != NoIndirectCalls) {
        for (Function &F: *M) {
            if (F.isDeclaration())
                continue;

            for (auto &instruction: instructions(F)) {
                if (auto SI = dyn_cast<StoreInst>(&instruction)) {
                    typeConfineInStore(SI);
                } else if (auto CastI = dyn_cast<CastInst>(&instruction)) {
                    typeConfineInCast(CastI);
                }
            }
        }
    }

    // Build metadata map
    for (const auto& function : *M) {
        for (const auto& instruction : instructions(function)) {
            /*if (isa<DbgValueInst>(instruction) || isa<DbgDeclareInst>(instruction)) {
                for (unsigned int i = 0, l = instruction.getNumOperands(); i < l; ++i) {
                    auto op = instruction.getOperand(i);
                    if (auto mdNode = dyn_cast_or_null<MDNode>(op)) {
                        visitMetadata(mdNode);
                    }
                }
            }*/
            SmallVector<pair<unsigned, MDNode*>, 4> mds;
            instruction.getAllMetadata(mds);
            for (const auto& entry : mds) {
                visitMetadata(entry.second);
            }
        }
    }

    metadata.clear();
#if 0
    for (const auto& [a, b] : typeAndOffsetToDebugTags) {
        LOG(LOG_INFO, a.first << "[" << a.second << "] -> " << b << "\n");
    }
#endif

	return false;
}

bool CallGraphPass::doFinalization(Module *M) {
	return false;
}

string CallGraphPass::computeDebugTag(const DISubroutineType* subroutineType) {
    // TODO: some caching?
    string differentiation;
    const auto &typeArray = subroutineType->getTypeArray();
    for (unsigned int i = 1, l = typeArray.size(); i < l; ++i) {
        auto diType = typeArray[i];
        while (auto current = dyn_cast_or_null<DIDerivedType>(diType)) {
            if (current->getTag() == 15 || current->getTag() == 38) {
                // pointer / constant
                differentiation += (current->getTag() == 15 ? '*' : 'C');
            } else {
                break;
            }

            diType = current->getBaseType();
        }
        differentiation += ",";
    }
    return differentiation;
}

void CallGraphPass::visitMetadata(const MDNode* mdNode) {
    if (!metadata.insert(mdNode).second)
        return;

    if (auto baseType = dyn_cast<DICompositeType>(mdNode)) {
        for (const auto *element: baseType->getElements()) {
            if (auto elementDerivedType = dyn_cast_or_null<DIDerivedType>(element)) {
                auto key = make_pair(baseType->getName(), elementDerivedType->getOffsetInBits());
                //if (typeAndOffsetToDebugTags.find(key) != typeAndOffsetToDebugTags.end())
                //    continue;
                //elementDerivedType->dump();
                if (auto pointerDerivedType = dyn_cast_or_null<DIDerivedType>(elementDerivedType->getBaseType())) {
                    //pointerDerivedType->dump();
                    if (auto subroutineType = dyn_cast_or_null<DISubroutineType>(pointerDerivedType->getBaseType())) {
                        typeAndOffsetToDebugTags[key] = computeDebugTag(subroutineType);
                    }
                }
            }
        }
    }

    for (unsigned int i = 0, l = mdNode->getNumOperands(); i < l; ++i) {
        if (auto otherMdNode = dyn_cast_or_null<MDNode>(mdNode->getOperand(i))) {
            visitMetadata(otherMdNode);
        }
    }
}

void CallGraphPass::generateDotGraphOutput(const Function* F, int depth) const {
    if (depth == 0)
        return;
    auto callersIt = Ctx->Callers.find(F);
    if (callersIt == Ctx->Callers.end())
        return;
    for (const auto* caller : callersIt->second) {
        auto callerFunction = caller->getFunction();
        OP << callerFunction->getName() << " -> " << F->getName();
        if (caller->isIndirectCall())
            OP << " [style=dotted]";
        OP << ";\n";
        generateDotGraphOutput(callerFunction, depth - 1);
    }
}

void CallGraphPass::doModulePass(Module *M) {
    // First, construct the call graph for the direct case
    for (auto& function : *M) {
        if(Ctx->UnifiedFuncSet.find(&function) == Ctx->UnifiedFuncSet.end())
            continue;

        // Unroll loops
#ifdef UNROLL_LOOP_ONCE
        unrollLoops(&function);
#endif

        // Collect callers and callees
        for (auto& instruction : instructions(function)) {
            // Map callsite to possible callees.
            if (auto CI = dyn_cast<CallInst>(&instruction)) {
                if (CI->isDebugOrPseudoInst() || CI->isInlineAsm() || CI->isIndirectCall())
                    continue;

                Function *CF = CI->getCalledFunction();
                Value *CV = CI->getCalledOperand();

                if (!CF) {
                    if (auto cast = dyn_cast</*BitCast*/Operator>(CV)) {
                        //cast->getOperand(0)->dump();
                        CF = dyn_cast<Function>(cast->getOperand(0));
                    }
                }

                if (CF) {
                    // Call external functions
                    if (CF->empty()) {
                        StringRef FName = CF->getName();
                        if (Function *GF = Ctx->GlobalFuncs[FName]) {
                            CF = GF;
                            //LOG(LOG_INFO, "Linked\n");
                        } else
                            Ctx->GlobalFuncs[FName] = CF; // Link the declarations against each other if it's not declared in any module
                    }

                    if (CF->isDeclaration()) {
                        //LOG(LOG_INFO, "is decl: " << CF->getName() << "\n");
                        Ctx->Callees[CI].insert(CF);
                        Ctx->Callers[CF].insert(CI);
                    } else {
                        // Use unified function
                        size_t fh = funcHash(CF);
                        CF = Ctx->UnifiedFuncMap[fh];
                        if (CF) {
                            Ctx->Callees[CI].insert(CF);
                            Ctx->Callers[CF].insert(CI);
                        }
                    }
                }
            }
        }
    }

    // Second, use type-analysis to conservatively find possible targets of indirect calls.
    for (auto& function : *M) {
		if(Ctx->UnifiedFuncSet.find(&function) == Ctx->UnifiedFuncSet.end())
			continue;

		// Collect callers and callees
        for (auto& instruction : instructions(function)) {
			// Map callsite to possible callees.
			if (auto CI = dyn_cast<CallInst>(&instruction)) {
                // Ignore intrinsics
                if (CI->getIntrinsicID() != Intrinsic::not_intrinsic || CI->isInlineAsm())
                    continue;

				if (CI->isIndirectCall()) {
                    auto& FS = Ctx->Callees[CI];
                    if (MLTA == FullMLTA)
					    findCalleesWithMLTA(CI, FS);
                    else if (MLTA == MatchSignatures)
					    findCalleesWithType(CI, FS);

					for (Function *Callee : FS)
						Ctx->Callers[Callee].insert(CI);
				}
			}
		}
	}

#if 0
    for (const auto& [callInstruction, possibleTargets] : Ctx->Callees) {
        callInstruction->dump();
        for (const auto& possibleTarget : possibleTargets) {
            LOG(LOG_INFO, "\tPossible target: " << possibleTarget->getName() << "\n");
        }
    }

    for (const auto& [function, callers] : Ctx->Callers) {
        LOG(LOG_INFO, "Function: " << function->getName() << "\n");
        for (const auto& caller : callers) {
            LOG(LOG_INFO, "\t" << caller->getFunction()->getName() << "\n");
        }
    }

    exit(1);
#endif

#if 0
    {
        OP << "digraph CG {\n";
        for (const auto& F : *M) {
            if (F.getName().equals("ecdh_cms_set_shared_info"))
                generateDotGraphOutput(&F, 4);
        }
        OP << "}\n";
    }
#endif
}
