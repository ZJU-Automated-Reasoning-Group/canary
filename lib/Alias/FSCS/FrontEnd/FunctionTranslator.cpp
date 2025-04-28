#include "Alias/FSCS/FrontEnd/CFG/FunctionTranslator.h"
#include "Alias/FSCS/FrontEnd/CFG/InstructionTranslator.h"
#include "Alias/FSCS/FrontEnd/CFG/PriorityAssigner.h"
#include "Alias/FSCS/Program/CFG/CFG.h"

#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

/**
 * @brief Translates LLVM basic blocks to pointer analysis CFG nodes
 * 
 * @param llvmFunc The LLVM function to translate
 * 
 * This method iterates through each basic block and each instruction
 * in the function, creating pointer analysis CFG nodes for relevant
 * instructions. It maintains mappings between instructions and CFG nodes,
 * and between basic blocks and their first/last CFG nodes.
 * 
 * Empty basic blocks (those without any pointer-relevant instructions)
 * are tracked separately for later processing.
 */
void FunctionTranslator::translateBasicBlock(const Function& llvmFunc)
{
	for (auto const& currBlock: llvmFunc)
	{
		tpa::CFGNode* startNode = nullptr;
		tpa::CFGNode* endNode = nullptr;

		for (auto const& inst: currBlock)
		{
			auto currNode = translator.visit(const_cast<Instruction&>(inst));
			if (currNode == nullptr)
				continue;
			else
			{
				instToNode[&inst] = currNode;
				nodeToInst[currNode] = &inst;
			}

			// Update the first node
			if (startNode == nullptr)
				startNode = currNode;
			// Chain the node with the last one
			if (endNode != nullptr)
				endNode->insertEdge(currNode);
			endNode = currNode;
		}

		assert((startNode == nullptr) == (endNode == nullptr));
		if (startNode != nullptr)
			bbToNode.insert(std::make_pair(&currBlock, std::make_pair(startNode, endNode)));
		else
			nonEmptySuccMap[&currBlock] = std::vector<tpa::CFGNode*>();
	}
}

/**
 * @brief Processes empty basic blocks to maintain control flow
 * 
 * Empty basic blocks (those without pointer-relevant instructions) still
 * need to be processed to maintain the correct control flow in the pointer
 * analysis CFG. This method performs a non-recursive DFS to find all
 * reachable non-empty blocks from each empty block, and records the
 * successors to maintain proper control flow.
 */
void FunctionTranslator::processEmptyBlock()
{
	auto processedEmptyBlock = SmallPtrSet<const BasicBlock*, 128>();
	for (auto& mapping: nonEmptySuccMap)
	{
		auto currBlock = mapping.first;
		auto succs = SmallPtrSet<tpa::CFGNode*, 16>();

		// The following codes try to implement a non-recursive DFS for finding all reachable non-empty blocks starting from an empty block
		auto workList = std::vector<const BasicBlock*>();
		workList.insert(workList.end(), succ_begin(currBlock), succ_end(currBlock));
		auto visitedEmptyBlock = SmallPtrSet<const BasicBlock*, 16>();
		visitedEmptyBlock.insert(currBlock);

		while (!workList.empty())
		{
			auto nextBlock = workList.back();
			workList.pop_back();

			if (bbToNode.count(nextBlock))
			{
				succs.insert(bbToNode[nextBlock].first);
			}
			else if (processedEmptyBlock.count(nextBlock))
			{
				auto& nextVec = nonEmptySuccMap[nextBlock];
				succs.insert(nextVec.begin(), nextVec.end());
			}
			else
			{
				for (auto itr = succ_begin(nextBlock), ite = succ_end(nextBlock); itr != ite; ++itr)
				{
					auto nextNextBlock = *itr;
					if (visitedEmptyBlock.count(nextNextBlock))
						continue;
					if (!bbToNode.count(nextNextBlock))
						visitedEmptyBlock.insert(nextNextBlock);
					workList.push_back(nextNextBlock);
				}
			}
		}

		processedEmptyBlock.insert(currBlock);
		mapping.second.insert(mapping.second.end(), succs.begin(), succs.end());
	}
}

/**
 * @brief Connects CFG nodes based on LLVM basic block control flow
 * 
 * @param entryBlock The entry block of the LLVM function
 * 
 * This method creates edges between CFG nodes to reflect the control flow
 * of the original LLVM function. For each basic block, it connects the last
 * node to the first nodes of all successor blocks. It also connects the
 * function's entry node to the start of the actual function body.
 */
void FunctionTranslator::connectCFGNodes(const BasicBlock& entryBlock)
{
	for (auto& mapping: bbToNode)
	{
		auto bb = mapping.first;
		auto lastNode = mapping.second.second;

		for (auto itr = succ_begin(bb), ite = succ_end(bb); itr != ite; ++itr)
		{
			auto nextBB = *itr;
			auto bbItr = bbToNode.find(nextBB);
			if (bbItr != bbToNode.end())
				lastNode->insertEdge(bbItr->second.first);
			else
			{
				assert(nonEmptySuccMap.count(nextBB));
				auto& vec = nonEmptySuccMap[nextBB];
				for (auto succNode: vec)
					lastNode->insertEdge(succNode);
			}
		}
	}

	// Connect the entry node with the main graph
	if (bbToNode.count(&entryBlock))
		cfg.getEntryNode()->insertEdge(bbToNode[&entryBlock].first);
	else
	{
		assert(nonEmptySuccMap.count(&entryBlock));
		auto& vec = nonEmptySuccMap[&entryBlock];
		for (auto node: vec)
			cfg.getEntryNode()->insertEdge(node);
	}
}

/**
 * @brief Creates def-use edges from a definition value to a use node
 * 
 * @param defVal The LLVM value (definition)
 * @param useNode The CFG node (use)
 * 
 * This helper method creates appropriate def-use edges for pointer values.
 * Global values, arguments, and constants are treated as defined at entry.
 * Instruction definitions are connected to their corresponding nodes.
 */
void FunctionTranslator::drawDefUseEdgeFromValue(const Value* defVal, tpa::CFGNode* useNode)
{
	assert(defVal != nullptr && useNode != nullptr);

	if (!defVal->getType()->isPointerTy())
		return;

	if (isa<GlobalValue>(defVal) || isa<Argument>(defVal) || isa<UndefValue>(defVal) || isa<ConstantPointerNull>(defVal))
	{
		// Nodes that use global values are def roots
		cfg.getEntryNode()->insertDefUseEdge(useNode);
	}
	else if (auto defInst = dyn_cast<Instruction>(defVal))
	{
		// For instructions, see if we have corresponding node attached to it
		if (auto defNode = instToNode[defInst])
			defNode->insertDefUseEdge(useNode);
		else
			errs() << "Failed to find node for instruction " << *defInst << "\n";	
	}
}

/**
 * @brief Constructs def-use chains for the entire function
 * 
 * Def-use chains track data dependencies between pointer definitions and uses.
 * This method iterates through all CFG nodes and creates appropriate def-use
 * edges based on the node type:
 * - Alloc nodes are defined at entry
 * - Copy nodes have def-use edges from all source values
 * - Offset nodes have def-use edges from the base pointer
 * - Load nodes have def-use edges from the source pointer
 * - Store nodes have def-use edges from both source and destination
 * - Call nodes have def-use edges from the function pointer and all arguments
 * - Return nodes have def-use edges from the return value
 */
void FunctionTranslator::constructDefUseChains()
{
	for (auto useNode: cfg)
	{
		switch (useNode->getNodeTag())
		{
			case CFGNodeTag::Entry:
				break;
			case CFGNodeTag::Alloc:
				cfg.getEntryNode()->insertDefUseEdge(useNode);
				break;
			case CFGNodeTag::Copy:
			{
				auto copyNode = static_cast<const CopyCFGNode*>(useNode);
				for (auto src: *copyNode)
				{
					auto defVal = src->stripPointerCasts();
					drawDefUseEdgeFromValue(defVal, useNode);
				}
				break;
			}
			case CFGNodeTag::Offset:
			{
				auto offsetNode = static_cast<const OffsetCFGNode*>(useNode);
				auto defVal = offsetNode->getSrc()->stripPointerCasts();
				drawDefUseEdgeFromValue(defVal, useNode);
				break;
			}
			case CFGNodeTag::Load:
			{
				auto loadNode = static_cast<const LoadCFGNode*>(useNode);
				auto defVal = loadNode->getSrc()->stripPointerCasts();
				drawDefUseEdgeFromValue(defVal, useNode);
				break;
			}
			case CFGNodeTag::Store:
			{
				auto storeNode = static_cast<const StoreCFGNode*>(useNode);
				auto srcVal = storeNode->getSrc()->stripPointerCasts();
				drawDefUseEdgeFromValue(srcVal, useNode);
				auto dstVal = storeNode->getDest()->stripPointerCasts();
				drawDefUseEdgeFromValue(dstVal, useNode);
				break;
			}
			case CFGNodeTag::Call:
			{
				auto callNode = static_cast<const CallCFGNode*>(useNode);
				auto funPtr = callNode->getFunctionPointer()->stripPointerCasts();
				drawDefUseEdgeFromValue(funPtr, useNode);
				for (auto arg: *callNode)
				{
					auto defVal = arg->stripPointerCasts();
					drawDefUseEdgeFromValue(defVal, useNode);
				}
				break;
			}
			case CFGNodeTag::Ret:
			{
				auto retNode = static_cast<const ReturnCFGNode*>(useNode);
				auto retVal = retNode->getReturnValue();
				if (retVal != nullptr)
				{
					auto defVal = retVal->stripPointerCasts();
					drawDefUseEdgeFromValue(defVal, useNode);
				}
				break;
			}
		}
	}
}

void FunctionTranslator::computeNodePriority()
{
	PriorityAssigner pa(cfg);
	pa.traverse();
}

void FunctionTranslator::detachStorePreservingNodes()
{
	for (auto node: cfg)
	{
		if (node->isAllocNode() || node->isCopyNode() || node->isOffsetNode())
			node->detachFromCFG();
	}
}

void FunctionTranslator::translateFunction(const Function& llvmFunc)
{
	// Scan the basic blocks and create the nodes first. We will worry about how to connect them later
	translateBasicBlock(llvmFunc);

	// Now the biggest problem are those "empty blocks" (i.e. blocks that do not contain any tpa::CFGNode). Those blocks may form cycles. So we need to know, in advance, what are the non empty successors of the empty blocks.
	processEmptyBlock();
	
	// Connect all the cfg nodes we've built
	connectCFGNodes(llvmFunc.getEntryBlock());

	// Draw def-use edges
	constructDefUseChains();

	// Compute the priority of each node
	computeNodePriority();

	// Detach all store-preserving nodes
	detachStorePreservingNodes();
}

}