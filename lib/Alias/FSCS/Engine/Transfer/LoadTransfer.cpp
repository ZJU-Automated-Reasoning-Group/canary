/*
Consider y = *x:
  pts(y) = ⋃ pts(o) for all o ∈ pts(x)
  If pts(x) = ∅, then pts(y) = {universal}

More concretely:
1. pts(x) = {o1, o2, ..., on}               // Points-to set of x
2. For each oi ∈ pts(x):
   pts(oi) = {p1, p2, ..., pm}              // Points-to set stored at object oi
3. pts(y) = ⋃ pts(oi) for all oi ∈ pts(x)   // Points-to set of y is the union of all points-to sets at objects pointed by x
4. If pts(x) = ∅, then pts(y) = {universal} // Conservative handling for unknown pointers
*/

#include "Alias/FSCS/Engine/GlobalState.h"
#include "Alias/FSCS/Engine/TransferFunction.h"
#include "Alias/FSCS/MemoryModel/MemoryManager.h"
#include "Alias/FSCS/MemoryModel/PointerManager.h"
#include <llvm/Support/raw_ostream.h>

namespace tpa
{

PtsSet TransferFunction::loadFromPointer(const Pointer* ptr, const Store& store)
{
	assert(ptr != nullptr);

	auto uObj = MemoryManager::getUniversalObject();
	auto srcSet = globalState.getEnv().lookup(ptr);
	if (!srcSet.empty())
	{
		std::vector<PtsSet> srcSets;
		srcSets.reserve(srcSet.size());  //
		
		for (auto obj: srcSet)
		{
			// lookup the store for the object
			auto objSet = store.lookup(obj);
			if (!objSet.empty())
			{
				srcSets.emplace_back(objSet);
				if (objSet.has(uObj))
					break;
			}
		}

		return PtsSet::mergeAll(srcSets);
	}

	return PtsSet::getSingletonSet(uObj);
}

void TransferFunction::evalLoadNode(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto ctx = pp.getContext();
	auto const& loadNode = static_cast<const LoadCFGNode&>(*pp.getCFGNode());

	// Debug - track contexts in load operations
	static size_t loadOpCount = 0;
	bool showDebug = loadOpCount < 20;
	loadOpCount++;

	auto& ptrManager = globalState.getPointerManager();
	auto srcPtr = ptrManager.getPointer(ctx, loadNode.getSrc());
	if (srcPtr == nullptr) {
		if (showDebug) {
			llvm::errs() << "DEBUG: Load src ptr is nullptr, ctx depth=" << ctx->size() << "\n";
		}
		return;
	}

	// Debug - verify the source pointer's context
	if (showDebug) {
		llvm::errs() << "DEBUG: [Load:" << loadOpCount << "] ctx depth=" << ctx->size() 
		             << ", src ptr ctx depth=" << srcPtr->getContext()->size();
		
		if (ctx != srcPtr->getContext()) {
			llvm::errs() << " (CONTEXT MISMATCH!)";
		}
		llvm::errs() << "\n";
	}

	//assert(srcPtr != nullptr && "LoadNode is evaluated before its src operand becomes available");
	auto dstPtr = ptrManager.getOrCreatePointer(ctx, loadNode.getDest());

	auto resSet = loadFromPointer(srcPtr, *localState);
	auto envChanged = globalState.getEnv().strongUpdate(dstPtr, resSet);
	
	if (envChanged)
		addTopLevelSuccessors(pp, evalResult);
	addMemLevelSuccessors(pp, *localState, evalResult);
}

}
