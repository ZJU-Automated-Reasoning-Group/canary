/*
 * SelectiveKCFA.cpp
 *
 * Selective k-CFA approach that allows different k values for different call sites
 * and allocation sites.
 */

#include "Alias/FSCS/Context/SelectiveKCFA.h"
#include "Alias/FSCS/Context/ProgramPoint.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <regex>

using namespace llvm;

namespace context
{

// Static members are defined in StaticFields.cpp
// No need to define them here again to avoid duplicate symbols

/**
 * Sets the context depth limit (k) for a specific call site
 *
 * @param callSite The call instruction
 * @param k The maximum context depth to use
 *
 * This allows tailoring context sensitivity on a per-call-site basis,
 * applying more precision where needed and less where it's not beneficial.
 */
void SelectiveKCFA::setCallSiteLimit(const Instruction* callSite, unsigned k)
{
    callSiteKLimits[callSite] = k;
}

/**
 * Sets the context depth limit (k) for a specific allocation site
 *
 * @param allocSite The allocation instruction
 * @param k The maximum context depth to use
 *
 * This allows applying different levels of precision to different
 * allocation sites based on their importance or complexity.
 */
void SelectiveKCFA::setAllocSiteLimit(const Instruction* allocSite, unsigned k)
{
    allocSiteKLimits[allocSite] = k;
}

/**
 * Gets the context depth limit for a specific call site
 *
 * @param callSite The call instruction
 * @return The k limit for this call site (or the default limit if not set)
 */
unsigned SelectiveKCFA::getCallSiteLimit(const Instruction* callSite)
{
    auto it = callSiteKLimits.find(callSite);
    return (it != callSiteKLimits.end()) ? it->second : defaultLimit;
}

/**
 * Gets the context depth limit for a specific allocation site
 *
 * @param allocSite The allocation instruction
 * @return The k limit for this allocation site (or the default limit if not set)
 */
unsigned SelectiveKCFA::getAllocSiteLimit(const Instruction* allocSite)
{
    auto it = allocSiteKLimits.find(allocSite);
    return (it != allocSiteKLimits.end()) ? it->second : defaultLimit;
}

/**
 * Pushes a new context element onto a context based on a program point
 *
 * @param pp The program point containing context and instruction information
 * @return The resulting context (new context or unchanged if limit reached)
 *
 * This method implements the context sensitivity policy for the analysis.
 */
const Context* SelectiveKCFA::pushContext(const ProgramPoint& pp)
{
    return pushContext(pp.getContext(), pp.getInstruction());
}

/**
 * Pushes a new context element onto a context
 *
 * @param ctx The current context
 * @param inst The instruction to add to the context
 * @return The resulting context (new context or unchanged if limit reached)
 *
 * This is the core method implementing selective k-CFA. It decides whether
 * to extend the context with a new element or to maintain the current context,
 * based on the configured k limits.
 */
const Context* SelectiveKCFA::pushContext(const Context* ctx, const Instruction* inst)
{
    // Determine the k limit to use for this instruction
    unsigned k = getCallSiteLimit(inst);
    
    // Check if the context size would exceed k
    assert(ctx->size() <= k);
    if (ctx->size() == k)
        return ctx;  // Do not add new context element, effectively merging paths
    else
        return Context::pushContext(ctx, inst);  // Add new context element
}

/**
 * Sets the k limit for all call sites within a function
 *
 * @param func The function to configure
 * @param k The k limit to apply to all call sites in the function
 *
 * This is a convenience method to apply the same k limit to all call sites
 * within a specific function.
 */
void SelectiveKCFA::setKLimitForFunctionCallSites(const Function* func, unsigned k)
{
    for (const_inst_iterator I = inst_begin(func), E = inst_end(func); I != E; ++I) {
        const Instruction* inst = &*I;
        if (isa<CallInst>(inst) || isa<InvokeInst>(inst)) {
            setCallSiteLimit(inst, k);
        }
    }
}

/**
 * Sets the k limit for all allocation sites within a function
 *
 * @param func The function to configure
 * @param k The k limit to apply to all allocation sites in the function
 *
 * This method identifies common allocation functions (malloc, new, etc.)
 * and applies a consistent k limit to them within a specific function.
 */
void SelectiveKCFA::setKLimitForFunctionAllocSites(const Function* func, unsigned k)
{
    for (const_inst_iterator I = inst_begin(func), E = inst_end(func); I != E; ++I) {
        const Instruction* inst = &*I;
        // Look for allocation instructions: malloc, calloc, new, etc.
        if (const CallInst* callInst = dyn_cast<CallInst>(inst)) {
            if (const Function* callee = callInst->getCalledFunction()) {
                StringRef name = callee->getName();
                if (name == "malloc" || name == "calloc" || name == "realloc" || 
                    name == "_Znwm" || name == "_Znam" ||  // new, new[]
                    name == "_Znwj" || name == "_Znaj") {  // new, new[] on 32-bit
                    setAllocSiteLimit(inst, k);
                }
            }
        }
    }
}

/**
 * Sets the k limit for call sites calling functions matching a name pattern
 *
 * @param module The LLVM module to analyze
 * @param pattern A regex pattern to match function names
 * @param k The k limit to apply to matching call sites
 *
 * This method enables targeted context sensitivity based on function name patterns,
 * allowing precision to be focused on specific libraries or functionality.
 */
void SelectiveKCFA::setKLimitForCallSitesByName(const Module* module, const std::string& pattern, unsigned k)
{
    std::regex nameRegex(pattern);
    
    for (const Function& F : *module) {
        // Skip declarations
        if (F.isDeclaration())
            continue;
            
        for (const_inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {
            const Instruction* inst = &*I;
            
            if (const CallInst* callInst = dyn_cast<CallInst>(inst)) {
                if (const Function* callee = callInst->getCalledFunction()) {
                    std::string calleeName = callee->getName().str();
                    if (std::regex_match(calleeName, nameRegex)) {
                        setCallSiteLimit(inst, k);
                    }
                }
            }
            else if (const InvokeInst* invokeInst = dyn_cast<InvokeInst>(inst)) {
                if (const Function* callee = invokeInst->getCalledFunction()) {
                    std::string calleeName = callee->getName().str();
                    if (std::regex_match(calleeName, nameRegex)) {
                        setCallSiteLimit(inst, k);
                    }
                }
            }
        }
    }
}

/**
 * Sets the k limit for call sites in a list of functions
 *
 * @param funcs A list of functions
 * @param k The k limit to apply to call sites in these functions
 *
 * This is a convenience method to apply the same k limit to call sites
 * across multiple functions.
 */
void SelectiveKCFA::setKLimitForFunctionsList(const std::vector<const Function*>& funcs, unsigned k)
{
    for (const Function* func : funcs) {
        setKLimitForFunctionCallSites(func, k);
    }
}

/**
 * Prints statistics about the configured context sensitivity
 *
 * @param os The output stream to print to
 *
 * This method provides insight into how the selective context sensitivity
 * is configured, showing the distribution of k values across call sites
 * and allocation sites.
 */
void SelectiveKCFA::printStats(raw_ostream& os)
{
    os << "SelectiveKCFA Configuration:\n";
    os << "  Default K limit: " << defaultLimit << "\n";
    os << "  Number of customized call sites: " << callSiteKLimits.size() << "\n";
    os << "  Number of customized allocation sites: " << allocSiteKLimits.size() << "\n";
    
    // Print distribution of k values for call sites
    std::unordered_map<unsigned, unsigned> callSiteKDistribution;
    for (const auto& pair : callSiteKLimits) {
        callSiteKDistribution[pair.second]++;
    }
    
    os << "  Call site K distribution:\n";
    for (const auto& pair : callSiteKDistribution) {
        os << "    K=" << pair.first << ": " << pair.second << " call sites\n";
    }
    
    // Print distribution of k values for allocation sites
    std::unordered_map<unsigned, unsigned> allocSiteKDistribution;
    for (const auto& pair : allocSiteKLimits) {
        allocSiteKDistribution[pair.second]++;
    }
    
    os << "  Allocation site K distribution:\n";
    for (const auto& pair : allocSiteKDistribution) {
        os << "    K=" << pair.first << ": " << pair.second << " allocation sites\n";
    }
}

} 