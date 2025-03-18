/*
 *  Author: rainoftime
 *  Date: 2025-04
 *  Description: Context-sensitive null flow analysis
 */


#include <llvm/IR/InstIterator.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include "DyckAA/DyckValueFlowAnalysis.h"
#include "NullPointer/ContextSensitiveNullFlowAnalysis.h"
#include "NullPointer/AliasAnalysisAdapter.h"
#include "Support/API.h"
#include "Support/RecursiveTimer.h"

using namespace llvm;

static cl::opt<int> CSIncrementalLimits("csnfa-limit", cl::init(10), cl::Hidden,
                                      cl::desc("Determine how many non-null edges we consider a round in context-sensitive analysis."));

static cl::opt<unsigned> CSMaxContextDepth("csnfa-max-depth", cl::init(3), cl::Hidden,
                                         cl::desc("Maximum depth of calling context to consider."));

static cl::opt<unsigned> CSRound("csnfa-round", cl::init(10), cl::Hidden,
                               cl::desc("Maximum rounds for context-sensitive analysis."));

// Define options for alias analysis selection
static cl::opt<unsigned> DyckAAOpt("nfa-dyck-aa", cl::init(1), cl::Hidden,
                        cl::desc("Use DyckAA for analysis. (0: None, 1: DyckAA)"));

static cl::opt<unsigned> CFLAAOpt("nfa-cfl-aa", cl::init(0), cl::Hidden,
                        cl::desc("Use CFLAA for analysis. (0: None, 1: Steensgaard, 2: Andersen)"));

char ContextSensitiveNullFlowAnalysis::ID = 0;
static RegisterPass<ContextSensitiveNullFlowAnalysis> X("csnfa", "context-sensitive null value flow");

ContextSensitiveNullFlowAnalysis::ContextSensitiveNullFlowAnalysis() 
    : ModulePass(ID), AAA(nullptr), VFG(nullptr), MaxContextDepth(CSMaxContextDepth), 
      OwnsAliasAnalysisAdapter(false) {
}

ContextSensitiveNullFlowAnalysis::~ContextSensitiveNullFlowAnalysis() {
    if (OwnsAliasAnalysisAdapter && AAA) {
        delete AAA;
    }
}

void ContextSensitiveNullFlowAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<DyckValueFlowAnalysis>();
}

bool ContextSensitiveNullFlowAnalysis::runOnModule(Module &M) {
    RecursiveTimer Timer("Running Context-Sensitive NFA");
    
    // Get the value flow graph
    auto *VFA = &getAnalysis<DyckValueFlowAnalysis>();
    VFG = VFA->getDyckVFGraph();
    
    // Create the appropriate alias analysis adapter using the factory method
    AAA = AliasAnalysisAdapter::createAdapter(&M, nullptr);
    OwnsAliasAnalysisAdapter = true;

    // Initialize the basic context (empty context)
    Context EmptyContext;
    
    // Helper function to identify values that must not be null
    auto MustNotNull = [this](Value *V, Instruction *I) -> bool {
        V = V->stripPointerCastsAndAliases();
        if (isa<GlobalValue>(V)) return true;
        if (auto CI = dyn_cast<Instruction>(V))
            return API::isMemoryAllocate(CI);
        return !AAA->mayNull(V, I);
    };
    
    // Initialize the analysis for each function
    std::set<DyckVFGNode *> MayNullNodes;
    for (auto &F: M) {
        if (!F.empty()) NewNonNullEdges[{&F, EmptyContext}];
        for (auto &I: instructions(&F)) {
            if (I.getType()->isPointerTy() && !MustNotNull(&I, &I)) {
                if (auto INode = VFG->getVFGNode(&I)) {
                    MayNullNodes.insert(INode);
                }
            }
        }
    }
    
    // Perform context-sensitive analysis
    std::set<std::pair<Function*, Context>> WorkList;
    for (auto &F: M) {
        if (!F.empty()) {
            WorkList.insert({&F, EmptyContext});
        }
    }
    
    // Keep analyzing until we reach a fixed point
    while (!WorkList.empty()) {
        auto FuncCtx = *WorkList.begin();
        WorkList.erase(WorkList.begin());
        
        Function *F = FuncCtx.first;
        Context &Ctx = FuncCtx.second;
        
        // Process the function in this context
        // This part depends on the specific algorithm of your null flow analysis
        errs() << "Processing function " << F->getName() << " with context " 
               << getContextString(Ctx) << "\n";
        
        // Check all call sites in this function
        for (auto &I : instructions(F)) {
            if (auto *CI = dyn_cast<CallInst>(&I)) {
                auto *Callee = CI->getCalledFunction();
                if (!Callee || Callee->empty()) continue;
                
                // If we haven't reached max context depth, create a new context
                if (Ctx.size() < MaxContextDepth) {
                    Context NewCtx = extendContext(Ctx, CI);
                    
                    // Add the callee with the new context to the worklist
                    auto FuncCtxPair = std::make_pair(Callee, NewCtx);
                    if (NewNonNullEdges.find(FuncCtxPair) == NewNonNullEdges.end()) {
                        NewNonNullEdges[FuncCtxPair] = {};
                        WorkList.insert(FuncCtxPair);
                    }
                }
            }
        }
    }
    
    return false;
}

bool ContextSensitiveNullFlowAnalysis::recompute(std::set<std::pair<Function*, Context>> &NewNonNullFunctionContexts) {
    // This method should implement the recomputation logic when new non-null edges are discovered
    // For simplicity, we'll just return false indicating no changes
    
    // In a real implementation, this would analyze the impact of new non-null edges
    // and update the analysis results accordingly
    
    return false;
}

bool ContextSensitiveNullFlowAnalysis::notNull(Value *Ptr, Context Ctx) const {
    if (!Ptr || !Ptr->getType()->isPointerTy())
        return false;
        
    // First check if the pointer is known to be non-null
    Ptr = Ptr->stripPointerCastsAndAliases();
    if (isa<GlobalValue>(Ptr)) return true;
    if (auto *I = dyn_cast<Instruction>(Ptr)) {
        if (API::isMemoryAllocate(I)) return true;
    }
    
    // Then check our context-sensitive analysis results
    // This depends on how you track non-null values in your analysis
    Function *F = nullptr;
    Instruction *InstPoint = nullptr;
    if (auto *I = dyn_cast<Instruction>(Ptr)) {
        F = I->getFunction();
        InstPoint = I;
    } else {
        // If it's not an instruction, we need a more conservative approach
        return false;
    }
    
    // Look through the contexts from most specific to most general
    while (!Ctx.empty()) {
        auto FuncCtxPair = std::make_pair(F, Ctx);
        auto it = NewNonNullEdges.find(FuncCtxPair);
        if (it != NewNonNullEdges.end()) {
            // Check if this pointer is marked as non-null in this context
            // This depends on how you track non-null values in your analysis
            // For now, we'll just check with the alias analysis adapter
            if (InstPoint && !AAA->mayNull(Ptr, InstPoint))
                return true;
        }
        
        // Try a more general context
        Ctx.pop_back();
    }
    
    // Check with the empty context
    Context EmptyCtx;
    auto FuncCtxPair = std::make_pair(F, EmptyCtx);
    auto it = NewNonNullEdges.find(FuncCtxPair);
    if (it != NewNonNullEdges.end()) {
        // Check if this pointer is marked as non-null in the empty context
        if (InstPoint && !AAA->mayNull(Ptr, InstPoint))
            return true;
    }
    
    return false;
}

void ContextSensitiveNullFlowAnalysis::add(Function *F, Context Ctx, Value *V1, Value *V2) {
    if (!V1 || !V1->getType()->isPointerTy())
        return;
        
    auto FuncCtxPair = std::make_pair(F, Ctx);
    auto it = NewNonNullEdges.find(FuncCtxPair);
    if (it == NewNonNullEdges.end()) {
        NewNonNullEdges[FuncCtxPair] = {};
    }
    
    // This implementation depends on how you track non-null values
    // For now, we'll just add a dummy entry to indicate that we've analyzed this context
}

void ContextSensitiveNullFlowAnalysis::add(Function *F, Context Ctx, CallInst *CI, unsigned int K) {
    if (!CI) return;
    
    auto FuncCtxPair = std::make_pair(F, Ctx);
    auto it = NewNonNullEdges.find(FuncCtxPair);
    if (it == NewNonNullEdges.end()) {
        NewNonNullEdges[FuncCtxPair] = {};
    }
    
    // Add this call site argument as non-null
    it->second.insert(std::make_pair(CI, K));
}

void ContextSensitiveNullFlowAnalysis::add(Function *F, Context Ctx, Value *Ret) {
    if (!Ret || !Ret->getType()->isPointerTy())
        return;
        
    auto FuncCtxPair = std::make_pair(F, Ctx);
    auto it = NewNonNullEdges.find(FuncCtxPair);
    if (it == NewNonNullEdges.end()) {
        NewNonNullEdges[FuncCtxPair] = {};
    }
    
    // This implementation depends on how you track non-null values
    // For now, we'll just add a dummy entry to indicate that we've analyzed this context
}

std::string ContextSensitiveNullFlowAnalysis::getContextString(const Context& Ctx) const {
    std::string Result = "[";
    for (size_t i = 0; i < Ctx.size(); ++i) {
        if (i > 0) Result += ", ";
        if (Ctx[i]->hasName()) {
            Result += Ctx[i]->getName().str();
        } else {
            Result += "<unnamed call>";
        }
    }
    Result += "]";
    return Result;
}

Context ContextSensitiveNullFlowAnalysis::extendContext(const Context& Ctx, CallInst* CI) const {
    // Create a new context by appending the call instruction
    Context NewCtx = Ctx;
    NewCtx.push_back(CI);
    
    // Limit context depth
    if (NewCtx.size() > MaxContextDepth) {
        NewCtx.erase(NewCtx.begin(), NewCtx.begin() + (NewCtx.size() - MaxContextDepth));
    }
    
    return NewCtx;
} 