#pragma once

#include "Alias/FSCS/Program/CFG/CFGNode.h"
#include "Alias/FSCS/Support/FunctionContext.h"
#include "Alias/FSCS/Context/ProgramPoint.h"
#include "Support/ADT/FIFOWorkList.h"
#include "Support/ADT/PriorityWorkList.h"
#include "Support/ADT/TwoLevelWorkList.h"

namespace tpa
{

template <typename CFGNodeComparator>
class IDFAWorkList
{
private:
	using GlobalWorkListType = util::FIFOWorkList<FunctionContext>;
	using LocalWorkListType = util::PriorityWorkList<const CFGNode*, CFGNodeComparator>;
	using WorkListType = util::TwoLevelWorkList<GlobalWorkListType, LocalWorkListType>;
	WorkListType workList;
public:
	using ElemType = context::ProgramPoint;

	IDFAWorkList() = default;

	void enqueue(const context::ProgramPoint& p)
	{
		// We need to retrieve the function and CFG node from the instruction
		// instead of calling getCFGNode() which doesn't exist in context::ProgramPoint
		const llvm::Instruction* inst = p.getInstruction();
		if (inst) {
			const llvm::Function* func = inst->getParent()->getParent();
			// Get the CFG node for this instruction
			auto* node = CFGNode::getForInstruction(inst);
			if (node) {
				workList.enqueue(std::make_pair(FunctionContext(p.getContext(), func), node));
			}
		}
	}

	context::ProgramPoint dequeue()
	{
		auto pair = workList.dequeue();
		// Find the instruction associated with the CFG node
		const llvm::Instruction* inst = pair.second->getInstruction();
		return context::ProgramPoint(pair.first.getContext(), inst);
	}

	context::ProgramPoint front()
	{
		auto pair = workList.front();
		// Find the instruction associated with the CFG node
		const llvm::Instruction* inst = pair.second->getInstruction();
		return context::ProgramPoint(pair.first.getContext(), inst);
	}

	bool empty() const { return workList.empty(); }
};

struct PriorityComparator
{
	bool operator()(const CFGNode* lhs, const CFGNode* rhs) const
	{
		return lhs->getPriority() < rhs->getPriority();
	}
};

struct PriorityReverseComparator
{
	bool operator()(const CFGNode* lhs, const CFGNode* rhs) const
	{
		return lhs->getPriority() > rhs->getPriority();
	}
};

using ForwardWorkList = IDFAWorkList<PriorityComparator>;
using BackwardWorkList = IDFAWorkList<PriorityReverseComparator>;

}
