#include "Alias/FSCS/Engine/GlobalState.h"
#include "Alias/FSCS/Engine/ContextSensitivity.h"
#include "Alias/FSCS/Engine/TransferFunction.h"
#include "Alias/FSCS/MemoryModel/MemoryManager.h"
#include "Alias/FSCS/MemoryModel/PointerManager.h"
#include "Alias/FSCS/Context/SelectiveKCFA.h"

namespace tpa
{

/**
 * @brief Transfer function for memory allocation instructions
 * 
 * @param ctx The context of the allocation
 * @param inst The allocation instruction
 * @param type The type layout for the allocated memory
 * @param isHeap Whether this is a heap allocation (true) or stack allocation (false)
 * @return true if the environment was changed by this allocation
 * 
 * This function:
 * 1. Applies context sensitivity policy to create a new context if needed
 * 2. Creates or retrieves a pointer for the allocation instruction
 * 3. Allocates an appropriate memory object (heap or stack)
 * 4. Updates the environment with the points-to relationship
 * 
 * This handles both stack allocations (alloca) and heap allocations (malloc, new, etc.).
 */
bool TransferFunction::evalMemoryAllocation(const context::Context* ctx, const llvm::Instruction* inst, const TypeLayout* type, bool isHeap)
{
	// Use the context that was provided (don't create a new one)
	// The context should already have been created at the call site
	auto allocCtx = ctx;
	auto ptr = globalState.getPointerManager().getOrCreatePointer(allocCtx, inst);

	auto mem = 
		isHeap?
		globalState.getMemoryManager().allocateHeapMemory(allocCtx, inst, type) :
		globalState.getMemoryManager().allocateStackMemory(allocCtx, inst, type);

	return globalState.getEnv().strongUpdate(ptr, PtsSet::getSingletonSet(mem));
}

/**
 * @brief Evaluates a CFG node representing a stack allocation
 * 
 * @param pp The program point for the allocation node
 * @param evalResult The evaluation result to populate
 * 
 * This method:
 * 1. Extracts information from the allocation CFG node
 * 2. Calls evalMemoryAllocation to create the memory object and update points-to info
 * 3. Adds successors to the evaluation result if the environment changed
 * 
 * AllocCFGNodes represent stack allocations (alloca instructions) in LLVM IR.
 * They create new memory objects and update the environment with points-to information.
 */
void TransferFunction::evalAllocNode(const ProgramPoint& pp, EvalResult& evalResult)
{
	auto allocNode = static_cast<const AllocCFGNode*>(pp.getCFGNode());
	auto envChanged = evalMemoryAllocation(pp.getContext(), allocNode->getDest(), allocNode->getAllocTypeLayout(), false);

	if (envChanged)
		addTopLevelSuccessors(pp, evalResult);
}

}
