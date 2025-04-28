#pragma once

#include "Alias/FSCS/Analysis/PointerAnalysis.h"
#include "Alias/FSCS/Context/SelectiveKCFA.h"

namespace llvm
{
    class Module;
}

namespace tpa
{

// A wrapper class that sets up selective k-CFA and runs pointer analysis
class SelectiveKCFAPointerAnalysis
{
public:
    // Run pointer analysis with selective k-CFA
    template <typename PtrAnalysisType>
    static void run(const llvm::Module& module)
    {
        // Set up selective k-CFA configuration
        setupSelectiveKCFA(&module);
        
        // Create and run the pointer analysis
        PtrAnalysisType ptrAnalysis;
        ptrAnalysis.runOnModule(module);
        
        // Print stats at the end
        context::SelectiveKCFA::printStats();
    }
    
private:
    // Configure SelectiveKCFA with default settings
    static void setupSelectiveKCFA(const llvm::Module* module);
};

} // namespace tpa 