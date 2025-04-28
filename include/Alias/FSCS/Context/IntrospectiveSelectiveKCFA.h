#pragma once

#include "Alias/FSCS/Context/SelectiveKCFA.h"
#include "Alias/FSCS/Analysis/PointerAnalysisQueries.h"
#include "Alias/FSCS/MemoryModel/PointerManager.h"
#include "Alias/FSCS/MemoryModel/MemoryManager.h"
#include "Alias/FSCS/Support/PtsSet.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace context
{

/**
 * IntrospectiveSelectiveKCFA - An extension of SelectiveKCFA that uses heuristics based on
 * a context-insensitive pre-analysis to determine which program elements should be refined.
 */
class IntrospectiveSelectiveKCFA : public SelectiveKCFA
{
private:
    // Metrics for call sites
    static std::unordered_map<const llvm::Instruction*, unsigned> callSiteInFlow;
    static std::unordered_map<const llvm::Function*, unsigned> methodTotalPointsToVolume;
    static std::unordered_map<const llvm::Function*, unsigned> methodMaxVarPointsTo;
    static std::unordered_map<const llvm::Function*, unsigned> methodMaxVarFieldPointsTo;
    
    // Metrics for allocation sites
    static std::unordered_map<const llvm::Instruction*, unsigned> allocSiteMaxFieldPointsTo;
    static std::unordered_map<const llvm::Instruction*, unsigned> allocSiteTotalFieldPointsTo;
    static std::unordered_map<const llvm::Instruction*, unsigned> allocSitePointedByVars;
    static std::unordered_map<const llvm::Instruction*, unsigned> allocSitePointedByObjs;
    
    // Thresholds for heuristics
    static unsigned thresholdPointedByVars;           // K in Heuristic A
    static unsigned thresholdInFlow;                  // L in Heuristic A
    static unsigned thresholdMaxVarFieldPointsTo;     // M in Heuristic A
    static unsigned thresholdTotalPointsToVolume;     // P in Heuristic B
    static unsigned thresholdFieldPtsMultipliedByVars; // Q in Heuristic B
    
    // Flag to indicate which heuristic to use (A or B)
    static bool useHeuristicA;
    
public:
    // Set which heuristic to use
    static void setHeuristic(bool useA) { useHeuristicA = useA; }
    
    // Set thresholds for Heuristic A
    static void setHeuristicAThresholds(unsigned K, unsigned L, unsigned M) {
        thresholdPointedByVars = K;
        thresholdInFlow = L;
        thresholdMaxVarFieldPointsTo = M;
    }
    
    // Set thresholds for Heuristic B
    static void setHeuristicBThresholds(unsigned P, unsigned Q) {
        thresholdTotalPointsToVolume = P;
        thresholdFieldPtsMultipliedByVars = Q;
    }
    
    // Compute metrics from a context-insensitive points-to analysis
    static void computeMetricsFromPreAnalysis(const tpa::PointerAnalysisQueries& queries, 
                                              const llvm::Module& module);
    
    // Apply heuristics to determine which call sites and allocation sites to refine
    static void applyHeuristics();
    
    // Utility methods to compute different metrics
    
    // Metric 1: Compute argument in-flow for a call site
    static unsigned computeCallSiteInFlow(const llvm::Instruction* callSite,
                                         const tpa::PointerAnalysisQueries& queries);
    
    // Metric 2: Compute total points-to volume for a method
    static unsigned computeMethodTotalPointsToVolume(const llvm::Function* function,
                                                    const tpa::PointerAnalysisQueries& queries);
    
    // Metric 2 variant: Compute max variable points-to set size for a method
    static unsigned computeMethodMaxVarPointsTo(const llvm::Function* function,
                                               const tpa::PointerAnalysisQueries& queries);
    
    // Metric 3: Compute max field points-to for an allocation site
    static unsigned computeAllocSiteMaxFieldPointsTo(const llvm::Instruction* allocSite,
                                                   const tpa::PointerAnalysisQueries& queries);
    
    // Metric 3 variant: Compute total field points-to for an allocation site
    static unsigned computeAllocSiteTotalFieldPointsTo(const llvm::Instruction* allocSite,
                                                     const tpa::PointerAnalysisQueries& queries);
    
    // Metric 4: Compute method's max var-field points-to
    static unsigned computeMethodMaxVarFieldPointsTo(const llvm::Function* function,
                                                   const tpa::PointerAnalysisQueries& queries);
    
    // Metric 5: Compute allocation site's pointed-by-vars
    static unsigned computeAllocSitePointedByVars(const llvm::Instruction* allocSite,
                                                const tpa::PointerAnalysisQueries& queries);
    
    // Metric 6: Compute allocation site's pointed-by-objs
    static unsigned computeAllocSitePointedByObjs(const llvm::Instruction* allocSite,
                                                const tpa::PointerAnalysisQueries& queries);
    
    // Print statistics about metrics and configuration
    static void printMetricsStats(llvm::raw_ostream& os = llvm::errs());
};

} // namespace context 