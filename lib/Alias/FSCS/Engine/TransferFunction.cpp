#include "Alias/FSCS/Engine/TransferFunction.h"
#include "Alias/FSCS/IO/Printer.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace context;
using namespace llvm;

namespace tpa
{

/**
 * Adds successor program points at the top level for a given program point
 *
 * @param pp The current program point
 * @param evalResult The evaluation result to add successors to
 *
 * This method adds control-flow successors for top-level propagation.
 * Top-level successors represent the next program points in the control-flow graph.
 */
void TransferFunction::addTopLevelSuccessors(const ProgramPoint& pp, EvalResult& evalResult)
{
	for (auto const succ: pp.getCFGNode()->uses())
		evalResult.addTopLevelProgramPoint(ProgramPoint(pp.getContext(), succ));
}

/**
 * Adds successor program points at the memory level for a given program point and store
 *
 * @param pp The current program point
 * @param store The current memory store
 * @param evalResult The evaluation result to add successors to
 *
 * This method adds control-flow successors for memory-level propagation.
 * These are used to propagate points-to information through memory operations.
 */
void TransferFunction::addMemLevelSuccessors(const ProgramPoint& pp, const Store& store, EvalResult& evalResult)
{
	for (auto const succ: pp.getCFGNode()->succs())
		evalResult.addMemLevelProgramPoint(ProgramPoint(pp.getContext(), succ), store);
}

/**
 * Evaluates a program point and computes its transfer function
 *
 * @param pp The program point to evaluate
 * @return The evaluation result containing new program points and stores
 *
 * This is the main transfer function implementation that delegates to
 * specific handlers based on the type of CFG node being processed.
 * It embodies the flow functions for points-to analysis.
 */
EvalResult TransferFunction::eval(const ProgramPoint& pp)
{
	//errs() << "Evaluating " << pp.getCFGNode()->getFunction().getName() << "::" << pp << "\n";
	EvalResult evalResult;

	switch (pp.getCFGNode()->getNodeTag())
	{
		case CFGNodeTag::Entry:
		{
			evalEntryNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Alloc:
		{
			evalAllocNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Copy:
		{
			evalCopyNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Offset:
		{
			evalOffsetNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Load:
		{
			if (localState != nullptr)
				evalLoadNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Store:
		{
			if (localState != nullptr)
				evalStoreNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Call:
		{
			if (localState != nullptr)
				evalCallNode(pp, evalResult);
			break;
		}
		case CFGNodeTag::Ret:
		{
			if (localState != nullptr)
				evalReturnNode(pp, evalResult);
			break;
		}
	}

	return evalResult;
}

}
