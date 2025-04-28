#include "Alias/FSCS/Engine/GlobalState.h"
#include "Alias/FSCS/Engine/TransferFunction.h"
#include "Alias/FSCS/MemoryModel/MemoryManager.h"
#include "Alias/FSCS/MemoryModel/PointerManager.h"
#include <llvm/Support/raw_ostream.h>

namespace tpa
{

void TransferFunction::strongUpdateStore(const MemoryObject* obj, PtsSet pSet, Store& store)
{
	if (!obj->isSpecialObject())
		store.strongUpdate(obj, pSet);
	// TODO: in the else branch, report NULL-pointer dereference to the user?
}

void TransferFunction::weakUpdateStore(PtsSet dstSet, PtsSet srcSet, Store& store)
{
	for (auto updateObj: dstSet)
	{
		if (!updateObj->isSpecialObject())
			store.weakUpdate(updateObj, srcSet);
	}
}

void TransferFunction::evalStore(const Pointer* dst, const Pointer* src, const ProgramPoint& pp, EvalResult& evalResult)
{
	// Debug - verify pointer contexts match the program point
	static size_t storeOpCount = 0;
	bool showDebug = storeOpCount < 20;
	storeOpCount++;
	
	if (showDebug) {
		auto ctx = pp.getContext();
		llvm::errs() << "DEBUG: [Store:" << storeOpCount << "] pp ctx depth=" << ctx->size()
		             << ", dst ptr ctx depth=" << dst->getContext()->size()
					 << ", src ptr ctx depth=" << src->getContext()->size();
					 
		if (ctx != dst->getContext() || ctx != src->getContext()) {
			llvm::errs() << " (CONTEXT MISMATCH!)";
		}
		llvm::errs() << "\n";
	}

	auto& env = globalState.getEnv();

	auto srcSet = env.lookup(src);
	if (srcSet.empty())
		return;

	auto dstSet = env.lookup(dst);
	if (dstSet.empty())
		return;

	auto& store = evalResult.getNewStore(*localState);

	auto dstObj = *dstSet.begin();
	// If the store target is precise and the target location is not unknown
	// TOOD: if the dstSet may grow, under what conditions can we perform the strong update here (is it because we are perfomring a flow-sensitive analysis)?
	if (dstSet.size() == 1 && !dstObj->isSummaryObject())
		strongUpdateStore(dstObj, srcSet, store);
	else
		weakUpdateStore(dstSet, srcSet, store);

	addMemLevelSuccessors(pp, store, evalResult);
}

void TransferFunction::evalStoreNode(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto ctx = pp.getContext();
	auto const& storeNode = static_cast<const StoreCFGNode&>(*pp.getCFGNode());

	auto& ptrManager = globalState.getPointerManager();
	auto srcPtr = ptrManager.getPointer(ctx, storeNode.getSrc());
	auto dstPtr = ptrManager.getPointer(ctx, storeNode.getDest());

	// Debug - track when pointers are missing
	static size_t storeNodeCount = 0;
	bool showDebug = storeNodeCount < 20;
	storeNodeCount++;
	
	if (srcPtr == nullptr || dstPtr == nullptr) {
		if (showDebug) {
			llvm::errs() << "DEBUG: [StoreNode:" << storeNodeCount << "] Missing pointer: "
			             << "src=" << (srcPtr ? "valid" : "null") 
						 << ", dst=" << (dstPtr ? "valid" : "null")
						 << ", ctx depth=" << ctx->size() << "\n";
		}
		return;
	}

	evalStore(dstPtr, srcPtr, pp, evalResult);
}

}
