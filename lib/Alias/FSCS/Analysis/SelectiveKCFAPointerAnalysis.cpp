#include "Alias/FSCS/Analysis/SelectiveKCFAPointerAnalysis.h"
#include "Alias/FSCS/Engine/Strategies/SimpleSelectiveKCFAStrategies.h"
#include "Alias/FSCS/Engine/Strategies/IntrospectiveContextSensitivity.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/CommandLine.h>

namespace tpa
{

// Enum to represent SelectiveKCFA strategy options
enum class SelectiveKCFAStrategy {
    Basic = 0,                // Basic heuristics
    Complex = 1,              // Complex heuristics based on function size
    CallFrequency = 2,        // Call frequency based
    IntrospectiveA = 3,       // Introspective analysis with heuristic A
    IntrospectiveB = 4        // Introspective analysis with heuristic B
};

// Command-line option for SelectiveKCFA strategy
static llvm::cl::opt<unsigned> SelectiveKCFAStrategyOpt(
    "selective-strategy", 
    llvm::cl::desc("SelectiveKCFA strategy selection:\n"
                   "  0 - Basic configuration with simple heuristics (default)\n"
                   "  1 - Complex configuration based on function size and allocation sites\n"
                   "  2 - Configuration based on call frequency analysis\n"
                   "  3 - Introspective analysis with heuristic A (pointed-by-vars, in-flow, max-var-field-points-to)\n"
                   "  4 - Introspective analysis with heuristic B (total points-to volume, field-points-to × pointed-by-vars)"),
    llvm::cl::init(0)
);

// Command-line options for Introspective heuristic A thresholds
static llvm::cl::opt<unsigned> IntrospectiveKOpt(
    "intro-k", 
    llvm::cl::desc("Threshold K for Introspective heuristic A: Allocation sites with pointed-by-vars > K will not be refined"),
    llvm::cl::init(50)
);

static llvm::cl::opt<unsigned> IntrospectiveLOpt(
    "intro-l", 
    llvm::cl::desc("Threshold L for Introspective heuristic A: Call sites with in-flow > L will not be refined"),
    llvm::cl::init(100)
);

static llvm::cl::opt<unsigned> IntrospectiveMOpt(
    "intro-m", 
    llvm::cl::desc("Threshold M for Introspective heuristic A: Call sites with max-var-field-points-to > M will not be refined"),
    llvm::cl::init(75)
);

// Command-line options for Introspective heuristic B thresholds
static llvm::cl::opt<unsigned> IntrospectivePOpt(
    "intro-p", 
    llvm::cl::desc("Threshold P for Introspective heuristic B: Methods with total-points-to-volume > P will not be refined"),
    llvm::cl::init(200)
);

static llvm::cl::opt<unsigned> IntrospectiveQOpt(
    "intro-q", 
    llvm::cl::desc("Threshold Q for Introspective heuristic B: Objects with (total-field-points-to × pointed-by-vars) > Q will not be refined"),
    llvm::cl::init(5000)
);

void SelectiveKCFAPointerAnalysis::setupSelectiveKCFA(const llvm::Module* module)
{
    // Set a basic default configuration
    context::SelectiveKCFA::setDefaultLimit(1);  // Default k = 1
    
    // Get the strategy from command line options
    SelectiveKCFAStrategy strategy = static_cast<SelectiveKCFAStrategy>(SelectiveKCFAStrategyOpt.getValue());
    
    switch(strategy) {
        case SelectiveKCFAStrategy::Complex:
            // Complex configuration with heuristics
            SelectiveKCFAStrategies::configureKLimitsByHeuristic(module);
            llvm::outs() << "Using SelectiveKCFA with complex heuristics based on function size and allocation sites\n";
            break;
            
        case SelectiveKCFAStrategy::CallFrequency:
            // Configuration based on call frequency
            SelectiveKCFAStrategies::configureKLimitsByCallFrequency(module);
            llvm::outs() << "Using SelectiveKCFA with call frequency heuristics\n";
            break;
            
        case SelectiveKCFAStrategy::IntrospectiveA:
            // Introspective analysis with heuristic A
            {
                llvm::outs() << "Setting up Introspective Analysis with Heuristic A\n";
                llvm::outs() << "This approach uses a context-insensitive pre-analysis to predict which program\n";
                llvm::outs() << "elements should not be refined based on cost metrics.\n\n";
                llvm::outs() << "Heuristic A (as defined in the Introspective paper):\n";
                llvm::outs() << "- Refine all allocation sites except those with a pointed-by-vars metric\n";
                llvm::outs() << "  higher than constant K (default=" << IntrospectiveKOpt.getValue() << ")\n";
                llvm::outs() << "- Refine all method call sites except those with either:\n";
                llvm::outs() << "  * an in-flow higher than constant L (default=" << IntrospectiveLOpt.getValue() << "), or\n";
                llvm::outs() << "  * a max var-field points-to higher than constant M (default=" << IntrospectiveMOpt.getValue() << ")\n\n";
                
                // Initialize introspective analysis with heuristic A
                IntrospectiveContextSensitivity introspective;
                
                // Configure thresholds from command line options
                unsigned thresholdK = IntrospectiveKOpt.getValue();
                unsigned thresholdL = IntrospectiveLOpt.getValue();
                unsigned thresholdM = IntrospectiveMOpt.getValue();
                
                // Initialize and configure with custom thresholds 
                introspective.initialize(module, true); // true = use heuristic A
                introspective.configureHeuristicA(thresholdK, thresholdL, thresholdM);
            }
            break;
            
        case SelectiveKCFAStrategy::IntrospectiveB:
            // Introspective analysis with heuristic B
            {
                llvm::outs() << "Setting up Introspective Analysis with Heuristic B\n";
                llvm::outs() << "This approach uses a context-insensitive pre-analysis to predict which program\n";
                llvm::outs() << "elements should not be refined based on cost metrics.\n\n";
                llvm::outs() << "Heuristic B (as defined in the Introspective paper):\n";
                llvm::outs() << "- Refine all method call sites except those that invoke methods with a\n";
                llvm::outs() << "  total points-to volume above constant P (default=" << IntrospectivePOpt.getValue() << ")\n";
                llvm::outs() << "- Refine all object allocations except those for which the product of\n";
                llvm::outs() << "  total field points-to and pointed-by-vars exceeds constant Q (default=" << IntrospectiveQOpt.getValue() << ")\n";
                llvm::outs() << "  This product can be seen as an object's total potential for weighing down the analysis.\n\n";
                
                // Initialize introspective analysis with heuristic B
                IntrospectiveContextSensitivity introspective;
                
                // Configure thresholds from command line options
                unsigned thresholdP = IntrospectivePOpt.getValue();
                unsigned thresholdQ = IntrospectiveQOpt.getValue();
                
                // Initialize and configure with custom thresholds
                introspective.initialize(module, false); // false = use heuristic B
                introspective.configureHeuristicB(thresholdP, thresholdQ);
            }
            break;
            
        case SelectiveKCFAStrategy::Basic:
        default:
            // Basic configuration with simple heuristics
            SelectiveKCFAStrategies::configureBasicStrategy(module);
            llvm::outs() << "Using SelectiveKCFA with basic heuristics\n";
            break;
    }
}

} // namespace tpa 