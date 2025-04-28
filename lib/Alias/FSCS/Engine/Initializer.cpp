#include "Alias/FSCS/Context/Context.h"
#include "Alias/FSCS/Context/KLimitContext.h"
#include "Alias/FSCS/Engine/GlobalState.h"
#include "Alias/FSCS/Engine/Initializer.h"
#include "Alias/FSCS/MemoryModel/MemoryManager.h"
#include "Alias/FSCS/MemoryModel/PointerManager.h"
#include "Alias/FSCS/Program/SemiSparseProgram.h"
#include "Alias/FSCS/Support/Memo.h"
#include <llvm/Support/raw_ostream.h>

namespace tpa
{

/**
 * @brief Initializes the pointer analysis with initial program state
 * 
 * @param initStore Initial memory store to use as starting point
 * @return ForwardWorkList Initialized worklist with program entry point
 * 
 * The Initializer sets up the initial state for pointer analysis, including:
 * 1. Setting up the entry context and CFG node
 * 2. Special handling for command-line arguments (argv and envp)
 * 3. Creating the initial program point with its associated store
 * 4. Creating the initial worklist for the analysis to process
 * 
 * This is a critical step in preparing the pointer analysis to analyze the
 * program starting from its entry function (usually main).
 */
ForwardWorkList Initializer::runOnInitState(Store&& initStore)
{
	ForwardWorkList workList;

	auto entryCtx = context::Context::getGlobalContext();
	auto entryCFG = globalState.getSemiSparseProgram().getEntryCFG();
	assert(entryCFG != nullptr);
	auto entryNode = entryCFG->getEntryNode();

	// Log entry context info
	llvm::errs() << "DEBUG: Initializing analysis with entry function " 
	             << entryCFG->getFunction().getName() 
				 << ", k=" << context::KLimitContext::getLimit() << "\n";

	// Set up argv
	auto& entryFunc = entryCFG->getFunction();
	if (entryFunc.arg_size() > 1)
	{
		// Create special points-to relationship for argv parameter
		auto argvValue = ++entryFunc.arg_begin();
		auto argvPtr = globalState.getPointerManager().getOrCreatePointer(entryCtx, argvValue);
		auto argvObj = globalState.getMemoryManager().allocateArgv(argvValue);
		globalState.getEnv().insert(argvPtr, argvObj);
		initStore.insert(argvObj, argvObj);

		if (entryFunc.arg_size() > 2)
		{
			// Create special points-to relationship for envp parameter
			auto envpValue = ++argvValue;
			auto envpPtr = globalState.getPointerManager().getOrCreatePointer(entryCtx, envpValue);
			auto envpObj = globalState.getMemoryManager().allocateEnvp(envpValue);
			globalState.getEnv().insert(envpPtr, envpObj);
			initStore.insert(envpObj, envpObj);
		}
	}

	// Create initial program point and add it to the worklist
	auto pp = ProgramPoint(entryCtx, entryNode);
	memo.update(pp, std::move(initStore));
	workList.enqueue(pp);
	
	llvm::errs() << "DEBUG: Initial program point created with context depth " 
	             << entryCtx->size() << ", node type EntryNode\n";

	return workList;
}

}
