#include "Annotation/Pointer/ExternalPointerTable.h"
#include "Annotation/Pointer/PointerEffect.h"
#include "Alias/FSCS/Engine/GlobalState.h"
#include "Alias/FSCS/Engine/ContextSensitivity.h"
#include "Alias/FSCS/Engine/TransferFunction.h"
#include "Alias/FSCS/MemoryModel/MemoryManager.h"
#include "Alias/FSCS/MemoryModel/PointerManager.h"
#include "Alias/FSCS/MemoryModel/Type/TypeLayout.h"
#include "Alias/FSCS/Program/SemiSparseProgram.h"
#include "Alias/FSCS/Context/SelectiveKCFA.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>

using namespace annotation;
using namespace context;
using namespace llvm;

namespace tpa
{

static const Value* getArgument(const CallCFGNode& callNode, const APosition& pos)
{
	auto inst = callNode.getCallSite();
	if (pos.isReturnPosition())
		return inst;

	// We can't just call callNode.getArgument(...) because there might be non-pointer args that are not included in callNode
	ImmutableCallSite cs(inst);
	assert(cs);
	
	auto argIdx = pos.getAsArgPosition().getArgIndex();
	// Check if argument index is out of bounds
	if (cs.arg_size() <= argIdx) {
		// Handle the case when argument index is out of bounds
		errs() << "Warning: Argument index " << argIdx << " out of bounds for call to " 
		       << (cs.getCalledFunction() ? cs.getCalledFunction()->getName() : "unknown function")
		       << " with " << cs.arg_size() << " arguments\n";
		return nullptr;
	}

	return cs.getArgument(argIdx)->stripPointerCasts();
}

static Type* getMallocType(const Instruction* callInst)
{
	assert(callInst != nullptr);

	// Use a safer approach to avoid potential null pointers
	PointerType* mallocType = nullptr;
	size_t numOfBitCastUses = 0;

	// Determine if CallInst has a bitcast use.
	for (auto user: callInst->users())
	{
		if (auto bcInst = dyn_cast<BitCastInst>(user))
		{
			Type* destType = bcInst->getDestTy();
			if (destType && destType->isPointerTy()) {
				mallocType = dyn_cast<PointerType>(destType);
				if (mallocType) {
					numOfBitCastUses++;
				}
			}
		}
		else if (isa<GetElementPtrInst>(user))
		{
			numOfBitCastUses++;
		}
	}

	// Malloc call has 1 bitcast use, so type is the bitcast's destination type.
	if (numOfBitCastUses == 1 && mallocType)
	{
		// Safely get element type
		Type* elemType = nullptr;
		try {
			elemType = mallocType->getElementType();
		} catch (...) {
			// If this fails, return nullptr
			return nullptr;
		}
		return elemType;
	}

	// Malloc call was not bitcast, so type is the malloc function's return type.
	if (numOfBitCastUses == 0)
	{
		Type* type = callInst->getType();
		if (type && type->isPointerTy())
		{
			auto ptrType = dyn_cast<PointerType>(type);
			if (ptrType)
			{
				// Safely get element type
				Type* elemType = nullptr;
				try {
					elemType = ptrType->getElementType();
				} catch (...) {
					// If this fails, return nullptr
					return nullptr;
				}
				return elemType;
			}
		}
	}

	// Type could not be determined. Return null as a conservative answer
	return nullptr;
}

static bool isSingleAlloc(const TypeLayout* typeLayout, const llvm::Value* sizeVal)
{
	if (sizeVal == nullptr)
		return false;

	if (auto cInt = dyn_cast<ConstantInt>(sizeVal))
	{
		auto size = cInt->getZExtValue();
		if (typeLayout->getSize() == 0 || size % typeLayout->getSize() != 0)
			return false;
		return size == typeLayout->getSize();
	}

	return false;
}

bool TransferFunction::evalMallocWithSize(const context::Context* ctx, const llvm::Instruction* dstVal, llvm::Type* mallocType, const llvm::Value* mallocSize)
{
	assert(ctx != nullptr && dstVal != nullptr);
	
	// Use the context that was passed in, don't create a new one
	// This context should already have been pushed at the call site
	auto allocCtx = ctx;

	const TypeLayout* typeLayout = nullptr;
	if (mallocType == nullptr)
		typeLayout = TypeLayout::getByteArrayTypeLayout();
	else
	{
		typeLayout = globalState.getSemiSparseProgram().getTypeMap().lookup(mallocType);
		assert(typeLayout != nullptr);
		if (!isSingleAlloc(typeLayout, mallocSize))
			// TODO: adjust type layout when mallocSize is known
			typeLayout = TypeLayout::getByteArrayTypeLayout();
	}

	return evalMemoryAllocation(allocCtx, dstVal, typeLayout, true);
}

bool TransferFunction::evalExternalAlloc(const context::Context* ctx, const CallCFGNode& callNode, const PointerAllocEffect& allocEffect)
{
	// TODO: add type hint to malloc-like calls
	auto dstVal = callNode.getDest();
	if (dstVal == nullptr)
		return false;
	
	// Apply context sensitivity based on the configured policy but make sure to use ctx
	// Don't create a new context here because we're already in a context from the call site
	auto callsite = callNode.getCallSite();
	
	auto mallocType = getMallocType(callNode.getCallSite());
	const Value* sizeVal = nullptr;
	if (allocEffect.hasSizePosition()) {
		sizeVal = getArgument(callNode, allocEffect.getSizePosition());
		// If we couldn't get the size argument, proceed with conservative assumptions
		if (sizeVal == nullptr) {
			errs() << "Warning: Could not retrieve size argument for allocation, using conservative allocation\n";
		}
	}

	// Use the context provided to us, not a newly created one
	return evalMallocWithSize(ctx, dstVal, mallocType, sizeVal);
}

void TransferFunction::evalMemcpyPtsSet(const MemoryObject* dstObj, const std::vector<const MemoryObject*>& srcObjs, size_t startingOffset, Store& store)
{
	auto& memManager = globalState.getMemoryManager();
	for (auto srcObj: srcObjs)
	{
		auto srcSet = store.lookup(srcObj);
		if (srcSet.empty())
			continue;

		auto offset = srcObj->getOffset() - startingOffset;
		auto tgtObj = memManager.offsetMemory(dstObj, offset);
		if (tgtObj->isSpecialObject())
			break;
		store.weakUpdate(tgtObj, srcSet);
	}
}

bool TransferFunction::evalMemcpyPointer(const Pointer* dst, const Pointer* src, Store& store)
{
	auto& env = globalState.getEnv();

	auto dstSet = env.lookup(dst);
	if (dstSet.empty())
		return false;
	auto srcSet = env.lookup(src);
	if (srcSet.empty())
		return false;

	auto& memManager = globalState.getMemoryManager();
	for (auto srcObj: srcSet)
	{
		auto srcObjs = memManager.getReachablePointerObjects(srcObj);
		for (auto dstObj: dstSet)
			evalMemcpyPtsSet(dstObj, srcObjs, srcObj->getOffset(), store);
	}
	return true;
}

bool TransferFunction::evalMemcpy(const context::Context* ctx, const CallCFGNode& callNode, Store& store, const APosition& dstPos, const APosition& srcPos)
{
	assert(dstPos.isArgPosition() && srcPos.isArgPosition() && "memcpy only operates on arguments");

	auto& ptrManager = globalState.getPointerManager();
	auto dstArg = getArgument(callNode, dstPos);
	if (dstArg == nullptr)
		return false;
	
	auto dstPtr = ptrManager.getPointer(ctx, dstArg);
	if (dstPtr == nullptr)
		return false;
	
	auto srcArg = getArgument(callNode, srcPos);
	if (srcArg == nullptr)
		return false;
	
	auto srcPtr = ptrManager.getPointer(ctx, srcArg);
	if (srcPtr == nullptr)
		return false;

	return evalMemcpyPointer(dstPtr, srcPtr, store);
}

PtsSet TransferFunction::evalExternalCopySource(const context::Context* ctx, const CallCFGNode& callNode, const CopySource& src)
{
	switch (src.getType())
	{
		case CopySource::SourceType::Value:
		{
			// Check if the argument exists
			auto argVal = getArgument(callNode, src.getPosition());
			if (argVal == nullptr)
				return PtsSet::getEmptySet();
			
			// Ensure we use the non-global context that was passed in
			auto ptr = globalState.getPointerManager().getPointer(ctx, argVal);
			if (ptr == nullptr)
				return PtsSet::getEmptySet();
			
			return globalState.getEnv().lookup(ptr);
		}
		case CopySource::SourceType::DirectMemory:
		{
			auto argVal = getArgument(callNode, src.getPosition());
			if (argVal == nullptr)
				return PtsSet::getEmptySet();
			
			// Ensure we use the non-global context that was passed in
			auto ptr = globalState.getPointerManager().getPointer(ctx, argVal);
			if (ptr == nullptr)
				return PtsSet::getEmptySet();
			
			return globalState.getEnv().lookup(ptr);
		}
		case CopySource::SourceType::Universal:
		{
			return PtsSet::getSingletonSet(MemoryManager::getUniversalObject());
		}
		case CopySource::SourceType::Null:
		{
			return PtsSet::getSingletonSet(MemoryManager::getNullObject());
		}
		case CopySource::SourceType::Static:
			// TODO: model "static" memory
			return PtsSet::getSingletonSet(MemoryManager::getUniversalObject());
		case CopySource::SourceType::ReachableMemory:
		{
			llvm_unreachable("ReachableMemory src should be handled earlier");
		}
	}
}

void TransferFunction::fillPtsSetWith(const Pointer* ptr, PtsSet srcSet, Store& store)
{
	auto pSet = globalState.getEnv().lookup(ptr);
	
	for (auto obj: pSet)
	{
		if (obj->isSpecialObject())
			continue;

		auto candidateObjs = globalState.getMemoryManager().getReachablePointerObjects(obj);
		for (auto tgtObj: candidateObjs)
			store.weakUpdate(tgtObj, srcSet);
	}
}

void TransferFunction::evalExternalCopyDest(const context::Context* ctx, const CallCFGNode& callNode, EvalResult& evalResult, const CopyDest& dest, PtsSet srcSet)
{
	// If the return value is not used, don't bother process it
	bool envChanged = false;
	if (!(callNode.getDest() == nullptr && dest.getPosition().isReturnPosition()))
	{
		auto argVal = getArgument(callNode, dest.getPosition());
		if (argVal == nullptr)
			return;
		
		// Ensure we're creating pointers with the context passed in, not the global context
		auto dstPtr = globalState.getPointerManager().getOrCreatePointer(ctx, argVal);
		
		switch (dest.getType())
		{
			case CopyDest::DestType::Value:
			{
				envChanged = globalState.getEnv().weakUpdate(dstPtr, srcSet);
				addMemLevelSuccessors(ProgramPoint(ctx, &callNode), *localState, evalResult);
				break;
			}
			case CopyDest::DestType::DirectMemory:
			{
				auto dstSet = globalState.getEnv().lookup(dstPtr);
				if (dstSet.empty())
					return;

				auto& store = evalResult.getNewStore(*localState);
				weakUpdateStore(dstSet, srcSet, store);
				addMemLevelSuccessors(ProgramPoint(ctx, &callNode), store, evalResult);
				break;
			}
			case CopyDest::DestType::ReachableMemory:
			{
				auto& store = evalResult.getNewStore(*localState);
				fillPtsSetWith(dstPtr, srcSet, store);
				addMemLevelSuccessors(ProgramPoint(ctx, &callNode), store, evalResult);
				break;
			}
		}
	}
}

void TransferFunction::evalExternalCopy(const context::Context* ctx, const CallCFGNode& callNode, EvalResult& evalResult, const PointerCopyEffect& copyEffect)
{
	auto const& src = copyEffect.getSource();
	auto const& dest = copyEffect.getDest();

	// Add debug output
	static size_t copyCount = 0;
	bool showDebug = copyCount < 20;
	copyCount++;
	
	if (showDebug) {
		llvm::errs() << "DEBUG: [ExternalCopy:" << copyCount << "] Processing copy with context depth=" 
		             << ctx->size() << ", src type=" << static_cast<int>(src.getType())
					 << ", dest type=" << static_cast<int>(dest.getType()) << "\n";
	}

	// Special case for memcpy: the source is not a single ptr/mem
	if (src.getType() == CopySource::SourceType::ReachableMemory)
	{
		assert(dest.getType() == CopyDest::DestType::ReachableMemory && "R src can only be assigned to R dest");

		auto& store = evalResult.getNewStore(*localState);
		auto storeChanged = evalMemcpy(ctx, callNode, store, dest.getPosition(), src.getPosition());

		if (storeChanged) {
			// Use the current context for the program point
			addMemLevelSuccessors(ProgramPoint(ctx, &callNode), store, evalResult);
			
			if (showDebug) {
				llvm::errs() << "DEBUG: [ExternalCopy:" << copyCount << "] Added mem-level successors for memcpy with context depth=" 
				             << ctx->size() << "\n";
			}
		} else if (showDebug) {
			llvm::errs() << "DEBUG: [ExternalCopy:" << copyCount << "] No store changes for memcpy\n";
		}
	}
	else
	{
		auto srcSet = evalExternalCopySource(ctx, callNode, src);
		if (!srcSet.empty()) {
			evalExternalCopyDest(ctx, callNode, evalResult, dest, srcSet);
			
			if (showDebug) {
				llvm::errs() << "DEBUG: [ExternalCopy:" << copyCount << "] Processed copy operation with non-empty source set, context depth=" 
				             << ctx->size() << "\n";
			}
		} else if (showDebug) {
			llvm::errs() << "DEBUG: [ExternalCopy:" << copyCount << "] Empty source set for copy operation\n";
		}
	}
}

void TransferFunction::evalExternalCallByEffect(const context::Context* ctx, const CallCFGNode& callNode, const PointerEffect& effect, EvalResult& evalResult)
{
	// Add debug output
	static size_t effectCount = 0;
	bool showDebug = effectCount < 20;
	effectCount++;
	
	if (showDebug) {
		llvm::errs() << "DEBUG: [ExternalEffect:" << effectCount << "] Processing effect type=" 
		             << static_cast<int>(effect.getType()) 
					 << " with context depth=" << ctx->size() << "\n";
	}
	
	switch (effect.getType())
	{
		case PointerEffectType::Alloc:
		{
			if (evalExternalAlloc(ctx, callNode, effect.getAsAllocEffect())) {
				// Use the current context for the program point
				addTopLevelSuccessors(ProgramPoint(ctx, &callNode), evalResult);
				
				if (showDebug) {
					llvm::errs() << "DEBUG: [ExternalEffect:" << effectCount << "] Added top-level successors with context depth=" 
								 << ctx->size() << "\n";
				}
			}
			// Preserve context for memory-level successors
			addMemLevelSuccessors(ProgramPoint(ctx, &callNode), *localState, evalResult);
			
			if (showDebug) {
				llvm::errs() << "DEBUG: [ExternalEffect:" << effectCount << "] Added mem-level successors with context depth=" 
				             << ctx->size() << "\n";
			}
			break;
		}
		case PointerEffectType::Copy:
		{
			evalExternalCopy(ctx, callNode, evalResult, effect.getAsCopyEffect());
			
			if (showDebug) {
				llvm::errs() << "DEBUG: [ExternalEffect:" << effectCount << "] Processed copy effect with context depth=" 
				             << ctx->size() << "\n";
			}
			break;
		}
		case PointerEffectType::Exit:
			if (showDebug) {
				llvm::errs() << "DEBUG: [ExternalEffect:" << effectCount << "] Processed exit effect with context depth=" 
				             << ctx->size() << "\n";
			}
			break;
	}
}

void TransferFunction::evalExternalCall(const context::Context* ctx, const CallCFGNode& callNode, const FunctionContext& fc, EvalResult& evalResult)
{
	// Use fc.getContext() directly instead of creating a new context
	// This ensures consistent context handling between internal and external calls
	auto newCtx = fc.getContext();
	
	// Add debug output to track context usage
	static size_t externalCallCount = 0;
	bool showDebug = externalCallCount < 20;
	externalCallCount++;
	
	if (showDebug) {
		llvm::errs() << "DEBUG: [ExternalCall:" << externalCallCount << "] Function " 
		             << fc.getFunction()->getName() 
					 << ", ctx depth=" << ctx->size() 
					 << ", new ctx depth=" << newCtx->size() << "\n";
	}
	
	auto summary = globalState.getExternalPointerTable().lookup(fc.getFunction()->getName());
	if (summary == nullptr)
	{
		llvm::errs() << "\nWarning: Cannot find annotation for external function:\n" << fc.getFunction()->getName() << "\n";
		llvm::errs() << "Treating as IGNORE. Add annotation to config file for more precise analysis.\n";
		
		// Treat unmodeled functions as no-ops by default instead of crashing
		// But preserve the context when adding successors
		addMemLevelSuccessors(ProgramPoint(newCtx, &callNode), *localState, evalResult);
		
		if (showDebug) {
			llvm::errs() << "DEBUG: [ExternalCall:" << externalCallCount << "] Added successors with context depth=" 
			             << newCtx->size() << "\n";
		}
		return;
	}

	// If the external func is a noop, we still need to propagate
	if (summary->empty())
	{
		addMemLevelSuccessors(ProgramPoint(newCtx, &callNode), *localState, evalResult);
		
		if (showDebug) {
			llvm::errs() << "DEBUG: [ExternalCall:" << externalCallCount << "] Empty summary, added successors with context depth=" 
			             << newCtx->size() << "\n";
		}
	}
	else
	{
		if (showDebug) {
			// Since PointerEffectSummary doesn't have a size() method, count effects manually
			size_t effectCount = 0;
			for (auto const& effect: *summary) {
				effectCount++;
			}
			
			llvm::errs() << "DEBUG: [ExternalCall:" << externalCallCount << "] Found " 
			             << effectCount << " effects to process with context depth=" 
						 << newCtx->size() << "\n";
		}
		
		for (auto const& effect: *summary)
			evalExternalCallByEffect(newCtx, callNode, effect, evalResult);
	}
}

}