#pragma once

#include "Alias/FSCS/Context/KLimitContext.h"
#include "Alias/FSCS/Context/SelectiveKCFA.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <string>

namespace tpa
{

// Global context sensitivity policy selection
class ContextSensitivityPolicy
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
    
    // The currently active policy (defaults to NoContext)
    static Policy activePolicy;
    
    // Configure a specific context sensitivity policy
    static void configurePolicy(Policy policy, const llvm::Module* module = nullptr) 
    {
        // Set the active policy
        activePolicy = policy;
        
        switch (policy) {
            case Policy::NoContext:
                context::KLimitContext::setLimit(0);
                break;
                
            case Policy::UniformKLimit:
                context::KLimitContext::setLimit(1); // Default to k=1
                break;
                
            case Policy::SelectiveKCFA:
                if (module) setupSelectiveKCFA(module);
                break;
        }
    }
    
    // Push a new context based on the active policy
    static const context::Context* pushContext(const context::Context* ctx, const llvm::Instruction* inst)
    {
        switch (activePolicy) {
            case Policy::NoContext:
                return ctx; // No new context, just return existing
                
            case Policy::UniformKLimit:
                return context::KLimitContext::pushContext(ctx, inst);
                
            case Policy::SelectiveKCFA:
                return context::SelectiveKCFA::pushContext(ctx, inst);
                
            default:
                return ctx; // Default to conservative behavior
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