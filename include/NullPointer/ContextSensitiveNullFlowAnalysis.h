/*
 *  Author: rainoftime
 *  Date: 2025-03
 *  Description: Context-sensitive null flow analysis
 */


#ifndef NULLPOINTER_CONTEXTSENSITIVENULLFLOWANALYSIS_H
#define NULLPOINTER_CONTEXTSENSITIVENULLFLOWANALYSIS_H

#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <map>
#include <set>
#include "DyckAA/DyckVFG.h"
#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckAA/DyckCallGraph.h"

using namespace llvm;

// Context representation for context-sensitive analysis
// A context is a sequence of call sites (represented by CallInst*)
typedef std::vector<CallInst*> Context;

class ContextSensitiveNullFlowAnalysis : public ModulePass {
private:
    DyckAliasAnalysis *DAA;
    DyckVFG *VFG;
    
    // Maps a context and a node to whether it's non-null
    std::map<std::pair<Context, DyckVFGNode*>, bool> ContextNonNullNodes;
    
    // Maps a context and an edge to whether it's a non-null edge
    std::map<std::tuple<Context, DyckVFGNode*, DyckVFGNode*>, bool> ContextNonNullEdges;
    
    // New non-null edges discovered in each function, grouped by context
    std::map<std::pair<Function*, Context>, std::set<std::pair<DyckVFGNode*, DyckVFGNode*>>> NewNonNullEdges;
    
    // Maximum context depth to consider (to prevent context explosion)
    unsigned MaxContextDepth;

public:
    static char ID;

    ContextSensitiveNullFlowAnalysis();
    ~ContextSensitiveNullFlowAnalysis() override;

    bool runOnModule(Module &M) override;
    void getAnalysisUsage(AnalysisUsage &AU) const override;

public:
    /// return true if some changes happen
    /// return false if nothing is changed
    bool recompute(std::set<std::pair<Function*, Context>> &NewNonNullFunctionContexts);

    /// update NewNonNullEdges so that we can call recompute()
    /// @{
    void add(Function*, Context, Value*, Value*);
    void add(Function*, Context, CallInst*, unsigned K); // for call
    void add(Function*, Context, Value*); // for return
    /// @}

    /// return true if the input value is not null in the given context
    bool notNull(Value*, const Context&) const;
    
    /// Helper method to get a context string for debugging
    std::string getContextString(const Context& Ctx) const;
    
    /// Helper method to create a new context by extending an existing one
    Context extendContext(const Context& Ctx, CallInst* CI) const;
};

#endif // NULLPOINTER_CONTEXTSENSITIVENULLFLOWANALYSIS_H 