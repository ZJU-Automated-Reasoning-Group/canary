/*
 *  Author: rainoftime
 *  Date: 2025-03
 *  Description: Context-sensitive null flow analysis
 */


#include <llvm/IR/InstIterator.h>
#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckAA/DyckValueFlowAnalysis.h"
#include "NullPointer/ContextSensitiveNullFlowAnalysis.h"
#include "Support/API.h"
#include "Support/RecursiveTimer.h"

static cl::opt<int> CSIncrementalLimits("csnfa-limit", cl::init(10), cl::Hidden,
                                      cl::desc("Determine how many non-null edges we consider a round in context-sensitive analysis."));

static cl::opt<unsigned> CSMaxContextDepth("csnfa-max-depth", cl::init(3), cl::Hidden,
                                         cl::desc("Maximum depth of calling context to consider."));

char ContextSensitiveNullFlowAnalysis::ID = 0;
static RegisterPass<ContextSensitiveNullFlowAnalysis> X("csnfa", "context-sensitive null value flow");

ContextSensitiveNullFlowAnalysis::ContextSensitiveNullFlowAnalysis() 
    : ModulePass(ID), DAA(nullptr), VFG(nullptr), MaxContextDepth(CSMaxContextDepth) {
}

ContextSensitiveNullFlowAnalysis::~ContextSensitiveNullFlowAnalysis() = default;

void ContextSensitiveNullFlowAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<DyckValueFlowAnalysis>();
    AU.addRequired<DyckAliasAnalysis>();
}

bool ContextSensitiveNullFlowAnalysis::runOnModule(Module &M) {
    RecursiveTimer DyckVFA("Running Context-Sensitive NFA");
    auto *VFA = &getAnalysis<DyckValueFlowAnalysis>();
    VFG = VFA->getDyckVFGraph();
    DAA = &getAnalysis<DyckAliasAnalysis>();

    // init may-null nodes with empty context (base context)
    Context EmptyContext;
    
    auto MustNotNull = [this](Value *V) -> bool {
        V = V->stripPointerCastsAndAliases();
        if (isa<GlobalValue>(V)) return true;
        if (auto CI = dyn_cast<Instruction>(V))
            return API::isMemoryAllocate(CI);
        return !DAA->mayNull(V);
    };
    
    std::set<DyckVFGNode *> MayNullNodes;
    for (auto &F: M) {
        if (!F.empty()) NewNonNullEdges[{&F, EmptyContext}];
        for (auto &I: instructions(&F)) {
            if (I.getType()->isPointerTy() && !MustNotNull(&I)) {
                if (auto INode = VFG->getVFGNode(&I)) {
                    MayNullNodes.insert(INode);
                }
            }
            for (unsigned K = 0; K < I.getNumOperands(); ++K) {
                auto *Op = I.getOperand(K);
                if (Op->getType()->isPointerTy() && !MustNotNull(Op)) {
                    if (auto OpNode = VFG->getVFGNode(Op)) {
                        MayNullNodes.insert(OpNode);
                    }
                }
            }
        }
    }

    // dfs to get all may-null nodes in the empty context
    std::vector<DyckVFGNode *> DFSStack(MayNullNodes.size());
    unsigned K = 0;
    for (auto *N: MayNullNodes) DFSStack[K++] = N;
    MayNullNodes.clear();
    std::set<DyckVFGNode *> &Visited = MayNullNodes;
    while (!DFSStack.empty()) {
        auto *Top = DFSStack.back();
        DFSStack.pop_back();
        if (Visited.count(Top)) continue;
        Visited.insert(Top);
        for (auto &T: *Top) if (!Visited.count(T.first)) DFSStack.push_back(T.first);
    }

    // Initialize all nodes as potentially null in the empty context
    for (auto *Node : Visited) {
        ContextNonNullNodes[{EmptyContext, Node}] = false;
    }
    
    return false;
}

bool ContextSensitiveNullFlowAnalysis::recompute(std::set<std::pair<Function*, Context>> &NewNonNullFunctionContexts) {
    std::set<std::pair<Context, DyckVFGNode*>> PossibleNonNullNodes;
    unsigned K = 0, Limits = CSIncrementalLimits < 0 ? UINT32_MAX : CSIncrementalLimits;
    
    // Process new non-null edges for each function and context
    for (auto &NIt: NewNonNullEdges) {
        auto FuncCtx = NIt.first;
        auto &EdgeSet = NIt.second;
        
        auto EIt = EdgeSet.begin();
        while (EIt != EdgeSet.end()) {
            if (++K > Limits) break;
            auto *Src = EIt->first;
            auto *Tgt = EIt->second;
            assert(Src && Tgt);
            
            // Get the context from the function-context pair
            const Context &Ctx = FuncCtx.second;
            
            // Check if target is already non-null in this context
            auto CtxNodePair = std::make_pair(Ctx, Tgt);
            auto NodeIt = ContextNonNullNodes.find(CtxNodePair);
            if (NodeIt == ContextNonNullNodes.end() || !NodeIt->second) {
                PossibleNonNullNodes.insert(CtxNodePair);
            }
            
            // Mark the edge as non-null in this context
            ContextNonNullEdges[std::make_tuple(Ctx, Src, Tgt)] = true;
            
            EIt = EdgeSet.erase(EIt);
        }
    }
    
    if (PossibleNonNullNodes.empty()) return false;

    unsigned OrigNonNullSize = 0;
    for (const auto &Entry : ContextNonNullNodes) {
        if (Entry.second) OrigNonNullSize++;
    }
    
    // Process possible non-null nodes
    std::vector<std::pair<Context, DyckVFGNode*>> WorkList(PossibleNonNullNodes.begin(), PossibleNonNullNodes.end());
    
    while (!WorkList.empty()) {
        auto CtxNode = WorkList.back();
        WorkList.pop_back();
        
        const Context &Ctx = CtxNode.first;
        DyckVFGNode *N = CtxNode.second;
        
        // Check if already marked as non-null
        auto NodeIt = ContextNonNullNodes.find(CtxNode);
        if (NodeIt != ContextNonNullNodes.end() && NodeIt->second) {
            continue;
        }
        
        // Check if all incoming edges of N are non-null edges in this context
        bool AllInNonNull = true;
        for (auto IIt = N->in_begin(), IE = N->in_end(); IIt != IE; ++IIt) {
            auto *In = IIt->first;
            
            // Check if the incoming node is non-null in this context
            auto InNodeIt = ContextNonNullNodes.find({Ctx, In});
            bool InNodeNonNull = (InNodeIt != ContextNonNullNodes.end() && InNodeIt->second);
            
            // Check if the edge is marked as non-null
            auto EdgeIt = ContextNonNullEdges.find(std::make_tuple(Ctx, In, N));
            bool EdgeNonNull = (EdgeIt != ContextNonNullEdges.end() && EdgeIt->second);
            
            if (!InNodeNonNull && !EdgeNonNull) {
                AllInNonNull = false;
                break;
            }
        }
        
        if (!AllInNonNull) continue;
        
        // Mark this node as non-null in this context
        ContextNonNullNodes[CtxNode] = true;
        
        // Add the function to the set of functions with new non-null nodes
        if (auto *NF = N->getFunction()) {
            NewNonNullFunctionContexts.insert({NF, Ctx});
        }
        
        // Add successors to the worklist
        for (auto &T: *N) {
            WorkList.push_back({Ctx, T.first});
        }
    }
    
    // Count new non-null nodes
    unsigned NewNonNullSize = 0;
    for (const auto &Entry : ContextNonNullNodes) {
        if (Entry.second) NewNonNullSize++;
    }
    
    return NewNonNullSize > OrigNonNullSize;
}

bool ContextSensitiveNullFlowAnalysis::notNull(Value *V, const Context &Ctx) const {
    assert(V);
    auto *N = VFG->getVFGNode(V);
    if (!N) return true;
    
    auto It = ContextNonNullNodes.find({Ctx, N});
    if (It != ContextNonNullNodes.end()) {
        return It->second;
    }
    
    // If not found in the specific context, try with empty context (context-insensitive fallback)
    Context EmptyContext;
    auto EmptyIt = ContextNonNullNodes.find({EmptyContext, N});
    if (EmptyIt != ContextNonNullNodes.end()) {
        return EmptyIt->second;
    }
    
    return false;
}

void ContextSensitiveNullFlowAnalysis::add(Function *F, Context Ctx, Value *V1, Value *V2) {
    auto *V1N = VFG->getVFGNode(V1);
    if(!V1N) return;
    auto *V2N = VFG->getVFGNode(V2);
    if (!V2N) return;
    
    // Limit context depth
    if (Ctx.size() > MaxContextDepth) {
        Ctx.erase(Ctx.begin(), Ctx.begin() + (Ctx.size() - MaxContextDepth));
    }
    
    NewNonNullEdges[{F, Ctx}].emplace(V1N, V2N);
}

void ContextSensitiveNullFlowAnalysis::add(Function *F, Context Ctx, CallInst *CI, unsigned int K) {
    auto *DCG = DAA->getDyckCallGraph();
    auto *DCGNode = DCG->getFunction(F);
    if (!DCGNode) return;
    auto *C = DCGNode->getCall(CI);
    if (!C) return;
    auto *Actual = CI->getArgOperand(K);
    
    // Limit context depth
    if (Ctx.size() > MaxContextDepth) {
        Ctx.erase(Ctx.begin(), Ctx.begin() + (Ctx.size() - MaxContextDepth));
    }
    
    if (auto *CC = dyn_cast<CommonCall>(C)) {
        auto *Callee = CC->getCalledFunction();
        if (K < Callee->arg_size()) {
            // Create a new context for the callee by adding this call site
            Context CalleeCtx = extendContext(Ctx, CI);
            add(F, Ctx, Actual, CC->getCalledFunction()->getArg(K));
        } else {
            assert(Callee->isVarArg());
        }
    } else {
        auto *PC = dyn_cast<PointerCall>(C);
        for (auto *Callee: *PC) {
            if (K < Callee->arg_size()) {
                // Create a new context for each potential callee
                Context CalleeCtx = extendContext(Ctx, CI);
                add(F, Ctx, Actual, Callee->getArg(K));
            } else {
                assert(Callee->isVarArg());
            }
        }
    }
}

void ContextSensitiveNullFlowAnalysis::add(Function *F, Context Ctx, Value *Ret) {
    if (!Ret) return;
    auto *RetN = VFG->getVFGNode(Ret);
    if (!RetN) return;
    
    // Limit context depth
    if (Ctx.size() > MaxContextDepth) {
        Ctx.erase(Ctx.begin(), Ctx.begin() + (Ctx.size() - MaxContextDepth));
    }
    
    auto &Set = NewNonNullEdges[{F, Ctx}];
    for (auto &TargetIt: *RetN) {
        Set.emplace(RetN, TargetIt.first);
    }
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