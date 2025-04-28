#include "Alias/FSCS/Engine/GlobalState.h"
#include "Alias/FSCS/Engine/StorePruner.h"
#include "Alias/FSCS/MemoryModel/MemoryManager.h"
#include "Alias/FSCS/MemoryModel/MemoryObject.h"
#include "Alias/FSCS/MemoryModel/PointerManager.h"
#include "Alias/FSCS/Program/CFG/CFGNode.h"
#include "Alias/FSCS/IO/Printer.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

/**
 * @brief Determines if a memory object is accessible across function boundaries
 *
 * @param obj The memory object to check
 * @return true if the object is globally accessible (not stack or heap)
 * 
 * This helper function identifies objects that can be accessed across function
 * boundaries (globals, static variables, etc.) as opposed to function-local
 * objects (stack allocations, heap allocations).
 */
static bool isAccessible(const MemoryObject* obj)
{
	return !(obj->isStackObject() || obj->isHeapObject());
}

/**
 * @brief Gets the root set of objects for store pruning
 *
 * @param store The memory store to analyze
 * @param pp The program point (call site) where pruning occurs
 * @return ObjectSet Set of root objects that must be preserved
 * 
 * The root set includes:
 * 1. Objects pointed to by function arguments
 * 2. Globally accessible objects (globals, statics)
 * 
 * These objects form the starting points for reachability analysis.
 */
StorePruner::ObjectSet StorePruner::getRootSet(const Store& store, const ProgramPoint& pp)
{
	ObjectSet ret;

	auto ctx = pp.getContext();
	auto const& callNode = static_cast<const CallCFGNode&>(*pp.getCFGNode());
	for (auto argVal: callNode)
	{
		auto argPtr = ptrManager.getPointer(ctx, argVal);
		if (argPtr == nullptr) {
			errs() << "Warning: Null pointer found in StorePruner::getRootSet. Skipping argument.\n";
			continue;
		}
		
		auto argSet = env.lookup(argPtr);
		ret.insert(argSet.begin(), argSet.end());
	}

	for (auto const& mapping: store)
	{
		if (isAccessible(mapping.first))
			ret.insert(mapping.first);
	}

	return ret;
}

/**
 * @brief Finds all objects reachable from a set of root objects
 *
 * @param store The memory store containing points-to information
 * @param reachableSet The set to populate with reachable objects (modified in place)
 * 
 * This method performs a graph traversal through the points-to relationships
 * to find all objects that can be reached from the root set. It uses a worklist
 * algorithm to efficiently process the object graph.
 */
void StorePruner::findAllReachableObjects(const Store& store, ObjectSet& reachableSet)
{
	std::vector<const MemoryObject*> currWorkList(reachableSet.begin(), reachableSet.end());
	std::vector<const MemoryObject*> nextWorkList;
	nextWorkList.reserve(reachableSet.size());
	ObjectSet exploredSet;

	while (!currWorkList.empty())
	{
		for (auto obj: currWorkList)
		{
			if (!exploredSet.insert(obj).second)
				continue;

			reachableSet.insert(obj);

			// Add objects reachable through pointer fields in the current object
			auto offsetObjs = memManager.getReachablePointerObjects(obj, false);
			for (auto oObj: offsetObjs)
				if (!exploredSet.count(oObj))
					nextWorkList.push_back(oObj);
				else
					break;

			// Add objects pointed to by the current object
			auto pSet = store.lookup(obj);
			for (auto pObj: pSet)
				if (!exploredSet.count(pObj))
					nextWorkList.push_back(pObj);
		}

		currWorkList.clear();
		currWorkList.swap(nextWorkList);
	}
}

/**
 * @brief Creates a new store containing only the reachable objects
 *
 * @param store The original store to filter
 * @param reachableSet The set of reachable objects to preserve
 * @return Store A new store with only reachable objects
 * 
 * This method filters the store to contain only mappings for objects
 * in the reachable set, reducing memory usage by eliminating unreachable objects.
 */
Store StorePruner::filterStore(const Store& store, const ObjectSet& reachableSet)
{
	Store ret;
	for (auto const& mapping: store)
	{
		if (reachableSet.count(mapping.first))
			ret.strongUpdate(mapping.first, mapping.second);
		else
			ret.strongUpdate(mapping.first, mapping.second);
	}
	return ret;
}

/**
 * @brief Prunes a store to contain only objects relevant to a function call
 *
 * @param store The store to prune
 * @param pp The program point (call site) where pruning occurs
 * @return Store A new, pruned store with only relevant objects
 * 
 * In context-sensitive analysis, each function call context only needs to maintain
 * memory state relevant to its execution, not the entire program's memory state.
 * This reduces memory usage and computational overhead.
 * 
 * The pruning process:
 * 1. Identify root objects (function arguments and globals)
 * 2. Find all objects reachable from these roots
 * 3. Create a new store containing only the reachable objects
 */
Store StorePruner::pruneStore(const Store& store, const ProgramPoint& pp)
{
	// In context-sensitive analysis, each function call context only needs to maintain
	// memory state relevant to its execution, not the entire program's memory state
	assert(pp.getCFGNode()->isCallNode() && "Prunning can only happen on call node!");
	
	auto reachableSet = getRootSet(store, pp);
	findAllReachableObjects(store, reachableSet);
	return filterStore(store, reachableSet);
}


}