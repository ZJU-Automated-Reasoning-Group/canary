#pragma once

#include "Alias/FSCS/Context/KLimitContext.h"
#include "Alias/FSCS/Context/SelectiveKCFA.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <string>

namespace tpa
{

// Helper class to set up different context sensitivity policies for pointer analysis
class ContextSensitivityManager
{
public:
    // Context sensitivity policies
    enum class Policy {
        // No context sensitivity (k=0)
        NoContext,
        
        // Standard k-limit context (all call sites have the same k)
        UniformKLimit,
        
        // Selective k-CFA (different call sites can have different k values)
        SelectiveKCFA
    };
    
    // Configure a specific context sensitivity policy
    static void configurePolicy(Policy policy, const llvm::Module* module) 
    {
        switch (policy) {
            case Policy::NoContext:
                context::KLimitContext::setLimit(0);
                break;
                
            case Policy::UniformKLimit:
                context::KLimitContext::setLimit(1); // Default to k=1
                break;
                
            case Policy::SelectiveKCFA:
                setupSelectiveKCFA(module);
                break;
        }
    }
    
private:
    // Configure selective k-CFA with default settings
    static void setupSelectiveKCFA(const llvm::Module* module) 
    {
        // Set default k limit for most call sites
        context::SelectiveKCFA::setDefaultLimit(1);
        
        // You can customize this based on your needs, e.g.:
        for (const llvm::Function& F : *module) {
            if (F.isDeclaration())
                continue;
                
            // Higher sensitivity for main and sensitive functions
            if (F.getName() == "main") {
                context::SelectiveKCFA::setKLimitForFunctionCallSites(&F, 3);
            }
            
            // Lower sensitivity for utility functions (if they match certain patterns)
            std::string name = F.getName().str();
            if (name.find("util") != std::string::npos || 
                name.find("helper") != std::string::npos) {
                context::SelectiveKCFA::setKLimitForFunctionCallSites(&F, 0);
            }
        }
    }
};

} // namespace tpa 