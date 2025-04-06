//===- ReachingDefinitions.h - Reaching definitions analysis ---*- C++ -*-===//
//
// Part of the Canary Project
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines a reaching definitions analysis for LLVM IR.
///
//===----------------------------------------------------------------------===//

#ifndef CANARY_REACHING_DEFINITIONS_H
#define CANARY_REACHING_DEFINITIONS_H

#include "Dataflow/DataflowAnalysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace canary {

// Domain for reaching definitions analysis
class ReachingDefsDomain {
public:
  using InstSetType = llvm::SmallDenseSet<const llvm::Instruction *, 8>;
  using ValueDefMapType = llvm::DenseMap<const llvm::Value *, InstSetType>;
  
  ReachingDefsDomain() = default;
  explicit ReachingDefsDomain(const ValueDefMapType &ReachingDefs) 
      : ReachingDefs(ReachingDefs) {}
  
  // Meet operation: union (this is a may analysis)
  ReachingDefsDomain meet(const ReachingDefsDomain &Other) const {
    // If one is empty, return the other (optimization)
    if (ReachingDefs.empty())
      return Other;
    if (Other.ReachingDefs.empty())
      return *this;
    
    // Choose the larger map as the base to minimize insertions
    if (ReachingDefs.size() > Other.ReachingDefs.size()) {
      ReachingDefsDomain Result(*this);
      
      for (const auto &Pair : Other.ReachingDefs) {
        const llvm::Value *V = Pair.first;
        const InstSetType &OtherDefs = Pair.second;
        
        // Get or create the set for this value
        auto &ResultDefs = Result.ReachingDefs[V];
        
        // Fast path if the set is empty
        if (ResultDefs.empty()) {
          ResultDefs = OtherDefs;
          continue;
        }
        
        // Otherwise insert elements
        ResultDefs.insert(OtherDefs.begin(), OtherDefs.end());
      }
      
      return Result;
    } else {
      ReachingDefsDomain Result(Other);
      
      for (const auto &Pair : ReachingDefs) {
        const llvm::Value *V = Pair.first;
        const InstSetType &ThisDefs = Pair.second;
        
        // Get or create the set for this value
        auto &ResultDefs = Result.ReachingDefs[V];
        
        // Fast path if the set is empty
        if (ResultDefs.empty()) {
          ResultDefs = ThisDefs;
          continue;
        }
        
        // Otherwise insert elements
        ResultDefs.insert(ThisDefs.begin(), ThisDefs.end());
      }
      
      return Result;
    }
  }
  
  // Equality test
  bool equals(const ReachingDefsDomain &Other) const {
    // Quick size check first
    if (ReachingDefs.size() != Other.ReachingDefs.size())
      return false;
    
    // Check that all values in this domain have the same definitions in Other
    for (const auto &Pair : ReachingDefs) {
      const llvm::Value *V = Pair.first;
      const InstSetType &Defs = Pair.second;
      
      auto It = Other.ReachingDefs.find(V);
      if (It == Other.ReachingDefs.end())
        return false;
        
      // Compare sizes first for fast inequality detection
      if (It->second.size() != Defs.size())
        return false;
        
      // Check each definition
      for (const llvm::Instruction *I : Defs) {
        if (!It->second.count(I))
          return false;
      }
    }
    
    return true;
  }
  
  // Getters and setters
  void addDefinition(const llvm::Value *V, const llvm::Instruction *I) {
    ReachingDefs[V].insert(I);
  }
  
  void removeDefinition(const llvm::Value *V, const llvm::Instruction *I) {
    auto It = ReachingDefs.find(V);
    if (It != ReachingDefs.end()) {
      It->second.erase(I);
      if (It->second.empty())
        ReachingDefs.erase(It);
    }
  }
  
  void removeAllDefinitions(const llvm::Value *V) {
    ReachingDefs.erase(V);
  }
  
  const InstSetType &getDefinitions(const llvm::Value *V) const {
    static const InstSetType EmptySet;
    auto It = ReachingDefs.find(V);
    return (It != ReachingDefs.end()) ? It->second : EmptySet;
  }
  
  const ValueDefMapType &getAllDefinitions() const {
    return ReachingDefs;
  }
  
private:
  ValueDefMapType ReachingDefs;
};

// Reaching definitions analysis (forward dataflow analysis)
class ReachingDefsAnalysis : public DataflowAnalysis<ReachingDefsDomain, Direction::Forward> {
public:
  explicit ReachingDefsAnalysis(llvm::Function &F);
  
  void initialize() override;
  
  // Transfer function for a single instruction
  ReachingDefsDomain transferInstruction(const llvm::Instruction &I, 
                                         const ReachingDefsDomain &State) const override;
  
  // Initial state: empty set of definitions
  ReachingDefsDomain initialState() const override;
  
  // Check if a basic block may affect reaching definitions
  bool mayAffectDomain(const llvm::BasicBlock &BB) const override {
    // A block affects reaching definitions if it contains any instruction that:
    // 1. Defines a value (non-void, non-terminator)
    // 2. Stores to memory
    // 3. Calls a function that may modify memory
    for (const auto &I : BB) {
      if (!I.isTerminator() && !I.isDebugOrPseudoInst() && 
          I.getType() != llvm::Type::getVoidTy(I.getContext()))
        return true;
      
      if (llvm::isa<llvm::StoreInst>(I))
        return true;
        
      if (auto *Call = llvm::dyn_cast<llvm::CallInst>(&I)) {
        if (Call->mayWriteToMemory())
          return true;
      }
    }
    return false;
  }
  
  // Print the reaching definitions for each basic block
  void print(llvm::raw_ostream &OS) const;
  
  // Return all reaching definitions at a given instruction
  ReachingDefsDomain getReachingDefs(const llvm::Instruction *I) const;
};

// Analysis pass for reaching definitions
class ReachingDefsAnalysisPass : public llvm::AnalysisInfoMixin<ReachingDefsAnalysisPass> {
  friend llvm::AnalysisInfoMixin<ReachingDefsAnalysisPass>;
  static llvm::AnalysisKey Key;
  
public:
  using Result = ReachingDefsAnalysis;
  
  ReachingDefsAnalysis run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
};

// Legacy pass for reaching definitions analysis
class ReachingDefsAnalysisWrapperPass : public llvm::FunctionPass {
public:
  static char ID;
  
  ReachingDefsAnalysisWrapperPass();
  
  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  void print(llvm::raw_ostream &OS, const llvm::Module *M = nullptr) const override;
  
  ReachingDefsAnalysis &getResult() { return *Result; }
  const ReachingDefsAnalysis &getResult() const { return *Result; }
  
private:
  std::unique_ptr<ReachingDefsAnalysis> Result;
};

} // end namespace canary

#endif // CANARY_REACHING_DEFINITIONS_H 