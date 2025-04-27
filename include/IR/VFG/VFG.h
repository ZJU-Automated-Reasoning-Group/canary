#pragma once

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <map>
#include <set>
#include <unordered_map>
#include "Support/CFG.h"
#include "Support/ADT/MapIterators.h"

using namespace llvm;

class AliasAnalysis;

class ModRefAnalysis;

class Call;

class VFGNode {
private:
    /// the value this node represents
    Value *V;

    /// labeled edge, 0 - epsilon, pos - call, neg - return
    /// @{
    using EdgeSetTy = std::set<std::pair<VFGNode *, int>>;
    EdgeSetTy Targets;
    EdgeSetTy Sources;
    /// @}

public:
    explicit VFGNode(Value *V) : V(V) {}

    void addTarget(VFGNode *N, int L = 0) {
        assert(N);
        this->Targets.emplace(N, L);
        N->Sources.emplace(this, L);
    }

    Value *getValue() const { return V; }

    Function *getFunction() const;

    EdgeSetTy::const_iterator begin() const { return Targets.begin(); }

    EdgeSetTy::const_iterator end() const { return Targets.end(); }

    EdgeSetTy::const_iterator in_begin() const { return Sources.begin(); }

    EdgeSetTy::const_iterator in_end() const { return Sources.end(); }
};

class VFG {
private:
    std::unordered_map<Value *, VFGNode *> ValueNodeMap;

public:
    VFG(AliasAnalysis *AA, ModRefAnalysis *MRA, Module *M);

    ~VFG();

    VFGNode *getVFGNode(Value *) const;

    value_iterator<std::unordered_map<Value *, VFGNode *>::iterator> node_begin() { return {ValueNodeMap.begin()}; }

    value_iterator<std::unordered_map<Value *, VFGNode *>::iterator> node_end() { return {ValueNodeMap.end()}; }

private:
    VFGNode *getOrCreateVFGNode(Value *);

    void connect(ModRefAnalysis *, Call *, Function *, CFG *);

    void buildLocalVFG(AliasAnalysis *, CFG *, Function *) const;

    void buildLocalVFG(Function &);
};

