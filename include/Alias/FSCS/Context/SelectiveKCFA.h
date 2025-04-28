#pragma once

#include "Alias/FSCS/Context/Context.h"
#include "Alias/FSCS/Context/ProgramPoint.h"

#include <llvm/IR/Instruction.h>
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
 * @class SelectiveKCFA
 * @brief Provides a selective k-limited context-sensitivity policy.
 *
 * This class implements a more flexible context-sensitivity policy than KLimitContext,
 * allowing different k values to be used for different parts of the program.
 * Specific call sites and allocation sites can have their own k limits, 
 * enabling a fine-grained balance between precision and performance.
 */
class SelectiveKCFA
{
private:
    static unsigned defaultLimit;  ///< Default k limit if no specific value is specified
    
    static std::unordered_map<const llvm::Instruction*, unsigned> callSiteKLimits;  ///< Maps call sites to their specific k values
    
    static std::unordered_map<const llvm::Instruction*, unsigned> allocSiteKLimits;  ///< Maps allocation sites to their specific k values
    
public:
    /**
     * @brief Sets the default k limit
     * @param k The default k limit to use
     */
    static void setDefaultLimit(unsigned k) { defaultLimit = k; }
    
    /**
     * @brief Gets the default k limit
     * @return The current default k limit
     */
    static unsigned getDefaultLimit() { return defaultLimit; }
    
    /**
     * @brief Sets a specific k limit for a call site
     * @param callSite The call instruction
     * @param k The k limit to use for this call site
     */
    static void setCallSiteLimit(const llvm::Instruction* callSite, unsigned k);
    
    /**
     * @brief Sets a specific k limit for an allocation site
     * @param allocSite The allocation instruction
     * @param k The k limit to use for this allocation site
     */
    static void setAllocSiteLimit(const llvm::Instruction* allocSite, unsigned k);
    
    /**
     * @brief Gets the k limit for a specific call site
     * @param callSite The call instruction
     * @return The k limit for this call site, or the default if not set
     */
    static unsigned getCallSiteLimit(const llvm::Instruction* callSite);
    
    /**
     * @brief Gets the k limit for a specific allocation site
     * @param allocSite The allocation instruction
     * @return The k limit for this allocation site, or the default if not set
     */
    static unsigned getAllocSiteLimit(const llvm::Instruction* allocSite);
    
    /**
     * @brief Creates a new context by pushing a call site, using the appropriate k limit
     * @param ctx The existing context
     * @param inst The call instruction to push
     * @return A pointer to the new (or existing) context
     */
    static const Context* pushContext(const Context* ctx, const llvm::Instruction* inst);
    
    /**
     * @brief Creates a new context by pushing a call site from a program point
     * @param pp The program point containing context and instruction
     * @return A pointer to the new (or existing) context
     */
    static const Context* pushContext(const ProgramPoint& pp);
    
    // Utility functions
    
    /**
     * @brief Sets the k limit for all call sites within a specific function
     * @param func The function whose call sites to configure
     * @param k The k limit to use
     */
    static void setKLimitForFunctionCallSites(const llvm::Function* func, unsigned k);
    
    /**
     * @brief Sets the k limit for all allocation sites within a specific function
     * @param func The function whose allocation sites to configure
     * @param k The k limit to use
     */
    static void setKLimitForFunctionAllocSites(const llvm::Function* func, unsigned k);
    
    /**
     * @brief Sets the k limit for call sites that match a name pattern
     * @param module The module to search in
     * @param pattern The name pattern to match against
     * @param k The k limit to use
     */
    static void setKLimitForCallSitesByName(const llvm::Module* module, const std::string& pattern, unsigned k);
    
    /**
     * @brief Sets the k limit for all call sites in a list of functions
     * @param funcs The list of functions whose call sites to configure
     * @param k The k limit to use
     */
    static void setKLimitForFunctionsList(const std::vector<const llvm::Function*>& funcs, unsigned k);
    
    /**
     * @brief Prints statistics about the current selective KCFA configuration
     * @param os The output stream to print to (defaults to stderr)
     */
    static void printStats(llvm::raw_ostream& os = llvm::errs());
};

} 