#include "Alias/FSCS/Engine/EvalResult.h"
#include "Alias/FSCS/Engine/GlobalState.h"
#include "Alias/FSCS/Engine/SemiSparsePropagator.h"
#include "Alias/FSCS/Engine/WorkList.h"
#include "Alias/FSCS/Program/CFG/CFG.h"
#include "Alias/FSCS/Support/Memo.h"
#include "Alias/FSCS/Context/Context.h"

#include "Alias/FSCS/IO/Printer.h"
#include <llvm/Support/raw_ostream.h>
using namespace llvm;

namespace tpa
{

namespace
{

/**
 * @brief Determines if a CFG node should be handled at the top level
 *
 * @param node The CFG node to check
 * @return true if the node is a top-level node
 * 
 * Top-level nodes include alloc, copy, and offset nodes, which primarily 
 * deal with pointer manipulations rather than memory accesses.
 */
bool isTopLevelNode(const CFGNode* node)
{
	return node->isAllocNode() || node->isCopyNode() || node->isOffsetNode();
}

}

/**
 * @brief Updates the memo with a new store and enqueues the program point if changed
 *
 * @param pp The program point to update
 * @param store The new store state
 * @return true if the memo was updated (store changed)
 * 
 * This helper method checks if the store has changed for a given program point.
 * If it has, the program point is enqueued for further processing.
 */
bool SemiSparsePropagator::enqueueIfMemoChange(const ProgramPoint& pp, const Store& store)
{
	// Debug contexts during propagation
	static size_t propagationCount = 0;
	bool showDebug = propagationCount < 50;
	propagationCount++;
	
	if (showDebug && propagationCount % 10 == 0) {
		errs() << "DEBUG: [Propagation:" << propagationCount << "] Context depth=" 
		       << pp.getContext()->size() << "\n";
	}

	if (memo.update(pp, store))
	{
		workList.enqueue(pp);
		return true;
	}
	else
		return false;
}

/**
 * @brief Propagates a top-level evaluation successor
 *
 * @param evalSucc The evaluation successor to propagate
 * 
 * Top-level propagation doesn't involve store merging. The program point
 * is simply enqueued for further processing. This is more efficient for
 * nodes that only manipulate pointers without accessing memory.
 */
void SemiSparsePropagator::propagateTopLevel(const EvalSuccessor& evalSucc)
{
	// Top-level successors: no store merging, just enqueue
	auto pp = evalSucc.getProgramPoint();
	
	// Critical: Ensure context is properly propagated
	static size_t topLevelPropCount = 0;
	bool showDebug = topLevelPropCount < 20;
	topLevelPropCount++;
	
	if (showDebug) {
		errs() << "DEBUG: [TopLevelProp:" << topLevelPropCount << "] Enqueueing program point with context depth="
		       << pp.getContext()->size() << "\n";
	}
	
	workList.enqueue(pp);
}

/**
 * @brief Propagates a memory-level evaluation successor
 *
 * @param evalSucc The evaluation successor to propagate
 * 
 * Memory-level propagation involves store merging. The new store is merged
 * with any existing store for the program point, and the program point is
 * enqueued only if the store changed. This approach improves efficiency
 * by avoiding redundant processing.
 */
void SemiSparsePropagator::propagateMemLevel(const EvalSuccessor& evalSucc)
{
	// Mem-level successors: store merging, enqueue if memo changed
	auto pp = evalSucc.getProgramPoint();
	auto node = pp.getCFGNode();
	
	// Critical: Debug context propagation in memory-level operations
	static size_t memLevelPropCount = 0;
	bool showDebug = memLevelPropCount < 20;
	memLevelPropCount++;
	
	if (showDebug) {
		errs() << "DEBUG: [MemLevelProp:" << memLevelPropCount << "] Processing program point with context depth="
		       << pp.getContext()->size() << "\n";
	}
	
	assert(!isTopLevelNode(node));
	assert(evalSucc.getStore() != nullptr);
	bool enqueued = enqueueIfMemoChange(pp, *evalSucc.getStore());

	if (showDebug && enqueued) {
		errs() << "DEBUG: [MemLevelProp:" << memLevelPropCount << "] Enqueued program point with context depth="
		       << pp.getContext()->size() << "\n";
	}
}

/**
 * @brief Propagates all successors from an evaluation result
 *
 * @param evalResult The evaluation result containing successors to propagate
 * 
 * This is the main propagation method that handles both top-level and memory-level
 * propagation. It implements the semi-sparse approach, which treats pointer
 * manipulations and memory accesses differently for better efficiency.
 * 
 * The semi-sparse approach is a hybrid between fully flow-sensitive analysis
 * (which is precise but expensive) and fully flow-insensitive analysis
 * (which is faster but less precise).
 */
void SemiSparsePropagator::propagate(const EvalResult& evalResult)
{
	for (auto const& evalSucc: evalResult)
	{
		if (evalSucc.isTopLevel())
			propagateTopLevel(evalSucc);
		else
			propagateMemLevel(evalSucc);
	}
}

}
