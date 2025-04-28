#include "Alias/FSCS/Engine/GlobalState.h"
#include "Alias/FSCS/Engine/WorkList.h"
#include "Alias/FSCS/MemoryModel/MemoryManager.h"
#include "Alias/FSCS/MemoryModel/Pointer.h"
#include "Alias/FSCS/MemoryModel/PointerManager.h"
#include "Alias/FSCS/Precision/PrecisionLossTracker.h"
#include "Alias/FSCS/Precision/TrackerGlobalState.h"
#include "Alias/FSCS/Precision/ValueDependenceTracker.h"
#include "Alias/FSCS/Program/SemiSparseProgram.h"

#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>

using namespace context;
using namespace llvm;

namespace tpa
{

/**
 * Helper function to get the function containing a value
 *
 * @param val The LLVM value
 * @return The function containing the value, or nullptr if not in a function
 */
static const Function* getFunction(const Value* val)
{
	if (auto arg = dyn_cast<Argument>(val))
		return arg->getParent();
	else if (auto inst = dyn_cast<Instruction>(val))
		return inst->getParent()->getParent();
	else
		return nullptr;
}

/**
 * Converts a list of pointers to a list of program points
 *
 * @param ptrs The list of pointers to convert
 * @return A list of program points corresponding to the pointer values
 *
 * This helper method converts pointers (which represent LLVM values and their contexts)
 * to program points (which represent CFG nodes and their contexts).
 */
PrecisionLossTracker::ProgramPointList PrecisionLossTracker::getProgramPointsFromPointers(const PointerList& ptrs)
{
	ProgramPointList list;
	list.reserve(ptrs.size());

	for (auto ptr: ptrs)
	{
		auto value = ptr->getValue();

		if (auto func = getFunction(value))
		{
			auto cfg = globalState.getSemiSparseProgram().getCFGForFunction(*func);
			assert(cfg != nullptr);

			auto node = cfg->getCFGNodeForValue(value);
			assert(node != nullptr);
			list.push_back(ProgramPoint(ptr->getContext(), node));
		}

	}

	return list;
}

namespace
{

/**
 * The ImprecisionTracker class identifies sources of imprecision in pointer analysis
 *
 * This class performs backward analysis to find program points where imprecision
 * is introduced. It compares points-to sets between dependent program points
 * to detect where precision is being lost.
 */
class ImprecisionTracker
{
private:
	TrackerGlobalState& globalState;

	/**
	 * Gets the points-to set for a value in a specific context
	 *
	 * @param ctx The context
	 * @param val The LLVM value
	 * @return The points-to set for the value in the given context
	 */
	PtsSet getPtsSet(const Context*, const Value*);
	
	/**
	 * Determines if one points-to set is more precise than another
	 *
	 * @param lhs The first points-to set
	 * @param rhs The second points-to set
	 * @return True if lhs is more precise than rhs, false otherwise
	 *
	 * A points-to set is considered more precise if it has fewer elements or
	 * doesn't contain the universal object when the other set does.
	 */
	bool morePrecise(const PtsSet&, const PtsSet&);

	/**
	 * Checks for precision loss between a call site and its callees
	 *
	 * @param pp The call site program point
	 * @param deps The set of dependent program points (modified in place)
	 *
	 * This method identifies return values that are more precise than
	 * the call destination, indicating precision loss at the call site.
	 */
	void checkCalleeDependencies(const ProgramPoint&, ProgramPointSet&);
	
	/**
	 * Checks for precision loss between a function entry and its callers
	 *
	 * @param pp The function entry program point
	 * @param deps The set of dependent program points (modified in place)
	 *
	 * This method identifies arguments that are more precise than the
	 * corresponding parameters, indicating precision loss at function boundaries.
	 */
	void checkCallerDependencies(const ProgramPoint&, ProgramPointSet&);
public:
	/**
	 * Constructor
	 *
	 * @param s The global state for the tracker
	 */
	ImprecisionTracker(TrackerGlobalState& s): globalState(s) {}

	/**
	 * Main algorithm to identify precision loss sources
	 *
	 * @param workList The backward worklist to process
	 *
	 * This method processes program points in a backward direction to
	 * identify dependencies and precision loss sources.
	 */
	void runOnWorkList(BackwardWorkList& workList);
};

/**
 * Gets the points-to set for a value in a specific context
 */
PtsSet ImprecisionTracker::getPtsSet(const Context* ctx, const Value* val)
{
	auto ptr = globalState.getPointerManager().getPointer(ctx, val);
	assert(ptr != nullptr);
	return globalState.getEnv().lookup(ptr);
}

/**
 * Main algorithm to identify precision loss sources
 *
 * Processes program points in a backward direction, checking for
 * precision loss at call sites and function entries. When precision
 * loss is detected, the origin point is recorded.
 */
void ImprecisionTracker::runOnWorkList(BackwardWorkList& workList)
{
	while (!workList.empty())
	{
		auto pp = workList.dequeue();
		if (!globalState.insertVisitedLocation(pp))
			continue;

		auto deps = ValueDependenceTracker(globalState.getCallGraph(), globalState.getSemiSparseProgram()).getValueDependencies(pp);

		auto node = pp.getCFGNode();
		if (node->isCallNode())
			checkCalleeDependencies(pp, deps);
		else if (node->isEntryNode())
			checkCallerDependencies(pp, deps);
		
		for (auto const& succ: deps)
			workList.enqueue(succ);
	}
}

/**
 * Determines if one points-to set is more precise than another
 *
 * A points-to set is considered more precise if:
 * 1. It doesn't contain the universal object when the other set does
 * 2. It has fewer elements than the other set
 */
bool ImprecisionTracker::morePrecise(const PtsSet& lhs, const PtsSet& rhs)
{
	auto uObj = MemoryManager::getUniversalObject();
	if (rhs.has(uObj))
		return !lhs.has(uObj);

	return lhs.size() < rhs.size();
}

/**
 * Checks for precision loss between a call site and its callees
 *
 * This method compares the points-to set of the call destination with
 * the points-to sets of the return values from callees. If any return
 * value has a more precise points-to set, the call site is identified
 * as a source of precision loss.
 */
void ImprecisionTracker::checkCalleeDependencies(const ProgramPoint& pp, ProgramPointSet& deps)
{
	assert(pp.getCFGNode()->isCallNode());
	auto callNode = static_cast<const CallCFGNode*>(pp.getCFGNode());
	auto dstVal = callNode->getDest();
	if (dstVal == nullptr)
		return;

	auto dstSet = getPtsSet(pp.getContext(), dstVal);
	assert(!dstSet.empty());

	ProgramPointSet newSet;
	bool needPrecision = false;
	for (auto const& retPoint: deps)
	{
		assert(retPoint.getCFGNode()->isReturnNode());
		auto retNode = static_cast<const ReturnCFGNode*>(retPoint.getCFGNode());
		auto retVal = retNode->getReturnValue();
		assert(retVal != nullptr);

		auto retSet = getPtsSet(retPoint.getContext(), retVal);
		assert(!retSet.empty());
		if (morePrecise(retSet, dstSet))
			needPrecision = true;
		else
			newSet.insert(retPoint);
	}

	if (needPrecision)
	{
		globalState.addImprecisionSource(pp);
		deps.swap(newSet);
	}
}

/**
 * Checks for precision loss between a function entry and its callers
 *
 * This method would compare the points-to sets of function parameters with
 * the points-to sets of corresponding arguments at call sites. 
 * (Not fully implemented in this code)
 */
void ImprecisionTracker::checkCallerDependencies(const ProgramPoint& pp, ProgramPointSet& deps)
{
	// TODO: Finish this
}


}

/**
 * Tracks sources of imprecision for a set of pointers
 *
 * @param ptrs The list of pointers to check for imprecision
 * @return A set of program points that are sources of imprecision
 *
 * This method performs backward analysis starting from the given pointers
 * to identify program points where precision is being lost.
 */
ProgramPointSet PrecisionLossTracker::trackImprecision(const PointerList& ptrs)
{
	ProgramPointSet ppSet;

	auto ppList = getProgramPointsFromPointers(ptrs);
	auto workList = BackwardWorkList();
	for (auto const& pp: ppList)
		workList.enqueue(pp);

	TrackerGlobalState trackerState(globalState.getPointerManager(), globalState.getMemoryManager(), globalState.getSemiSparseProgram(), globalState.getEnv(), globalState.getCallGraph() ,globalState.getExternalPointerTable(), ppSet);
	ImprecisionTracker(trackerState).runOnWorkList(workList);

	return ppSet;
}

}
