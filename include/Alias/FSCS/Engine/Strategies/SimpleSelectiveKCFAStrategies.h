#pragma once

#include "Alias/FSCS/Context/SelectiveKCFA.h"

namespace llvm
{
    class Module;
    class Function;
}

namespace tpa
{

/**
 * SelectiveKCFAStrategies - A collection of strategies for configuring SelectiveKCFA
 * 
 * This class provides different ways to set k-limits for call sites and allocation sites
 * based on various heuristics.
 */
class SelectiveKCFAStrategies
{
public:
    /**
     * Basic strategy that uses simple heuristics to configure k-limits
     * 
     * Sets higher k values for small functions and lower k values for large functions.
     * Also sets a higher k value for important functions like main.
     */
    static void configureBasicStrategy(const llvm::Module* module);
    
    /**
     * Complex strategy that considers function size and allocation sites
     * 
     * Sets k-limits based on function size and the number of allocation sites.
     * Small functions get higher k values (3), large functions get lower k values (0).
     * Functions with many allocations get lower k values for their allocation sites.
     */
    static void configureKLimitsByHeuristic(const llvm::Module* module);
    
    /**
     * Strategy that sets k-limits based on call frequency
     * 
     * Analyzes how frequently each function is called and sets k-limits accordingly.
     * Frequently called functions get lower k values to prevent context explosion.
     * Rarely called functions get higher k values for better precision.
     */
    static void configureKLimitsByCallFrequency(const llvm::Module* module);
    
    /**
     * Utility function to print statistics about the selected SelectiveKCFA strategy
     */
    static void printStats(llvm::raw_ostream& os = llvm::errs());
};

} // namespace tpa 