#include "Alias/FSCS/Analysis/GlobalPointerAnalysis.h"
#include "Alias/FSCS/Analysis/SemiSparsePointerAnalysis.h"
#include "Alias/FSCS/Engine/GlobalState.h"
#include "Alias/FSCS/Engine/Initializer.h"
#include "Alias/FSCS/Engine/SemiSparsePropagator.h"
#include "Alias/FSCS/Engine/TransferFunction.h"
#include "Alias/FSCS/Program/SemiSparseProgram.h"
#include "Alias/FSCS/Context/Context.h"
#include "Alias/FSCS/Context/KLimitContext.h"
#include "Alias/FSCS/Support/DataFlowAnalysis.h"
#include <llvm/Support/raw_ostream.h>

namespace tpa
{

void SemiSparsePointerAnalysis::runOnProgram(const SemiSparseProgram& ssProg)
{
	auto initStore = Store();
	
	// Debug output for context settings 
	llvm::errs() << "DEBUG: Starting pointer analysis with k-limit: " 
	             << context::KLimitContext::getLimit() 
				 << ", global value context preservation: " 
				 << (ptrManager.getPreserveGlobalValueContexts() ? "enabled" : "disabled")
				 << "\n";
				 
	// Run the global pointer analysis to set up the initial environment
	std::tie(env, initStore) = GlobalPointerAnalysis(ptrManager, memManager, ssProg.getTypeMap()).runOnModule(ssProg.getModule());

	// Set up the global state and perform the analysis
	auto globalState = GlobalState(ptrManager, memManager, ssProg, extTable, env);
	auto dfa = util::DataFlowAnalysis<GlobalState, Memo, TransferFunction, SemiSparsePropagator>(globalState, memo);
	
	// Debug output for context state before analysis
	size_t globalPtrCount = 0;
	size_t nonGlobalPtrCount = 0;
	for (const auto* ptr : ptrManager.getAllPointers()) {
		if (ptr->getContext()->isGlobalContext()) {
			globalPtrCount++;
		} else {
			nonGlobalPtrCount++;
		}
	}
	
	llvm::errs() << "DEBUG: Before analysis: " << globalPtrCount << " pointers with global context, "
	             << nonGlobalPtrCount << " pointers with non-global context\n";
	
	// Run the data flow analysis
	dfa.runOnInitialState<Initializer>(std::move(initStore));
	
	// Debug output for context state after analysis
	globalPtrCount = 0;
	nonGlobalPtrCount = 0;
	for (const auto* ptr : ptrManager.getAllPointers()) {
		if (ptr->getContext()->isGlobalContext()) {
			globalPtrCount++;
		} else {
			nonGlobalPtrCount++;
		}
	}
	
	llvm::errs() << "DEBUG: After analysis: " << globalPtrCount << " pointers with global context, "
	             << nonGlobalPtrCount << " pointers with non-global context\n";
}

PtsSet SemiSparsePointerAnalysis::getPtsSetImpl(const Pointer* ptr) const
{
	return env.lookup(ptr);
}

}