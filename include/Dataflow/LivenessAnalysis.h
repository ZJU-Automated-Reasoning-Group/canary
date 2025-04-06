//===- LivenessAnalysis.h - Analysis for live variables -----------*- C++ -*-===//
//
// Part of the Canary Project
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines a liveness analysis for LLVM IR.
///
//===----------------------------------------------------------------------===//

#ifndef CANARY_LIVENESS_ANALYSIS_H
#define CANARY_LIVENESS_ANALYSIS_H

#include "Dataflow/DataflowAnalysis.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace canary {

// Domain for liveness analysis: set of live variables
class LivenessDomain {
public:
  using ValueSetType = llvm::SmallDenseSet<const llvm::Value *, 16>;
  
  LivenessDomain() = default;
  explicit LivenessDomain(const ValueSetType &LiveVars) : LiveVars(LiveVars) {}
  
  // Meet operation: union (this is a may analysis)
  LivenessDomain meet(const LivenessDomain &Other) const {
    // If one is empty, return the other (optimization)
    if (LiveVars.empty())
      return Other;
    if (Other.LiveVars.empty())
      return *this;
      
    // If the other has fewer elements, insert them into a copy of this
    // Otherwise, insert this into a copy of the other
    if (LiveVars.size() > Other.LiveVars.size()) {
      LivenessDomain Result(*this);
      Result.LiveVars.insert(Other.LiveVars.begin(), Other.LiveVars.end());
      return Result;
    } else {
      LivenessDomain Result(Other);
      Result.LiveVars.insert(LiveVars.begin(), LiveVars.end());
      return Result;
    }
  }
  
  // Equality test
  bool equals(const LivenessDomain &Other) const {
    // Quick size check first
    if (LiveVars.size() != Other.LiveVars.size())
      return false;
      
    // Check if all elements in this are in Other
    for (const llvm::Value *V : LiveVars) {
      if (!Other.LiveVars.count(V))
        return false;
    }
    
    return true;
  }
  
  // Getters and setters
  void addLiveVar(const llvm::Value *V) { LiveVars.insert(V); }
  void removeLiveVar(const llvm::Value *V) { LiveVars.erase(V); }
  bool isLive(const llvm::Value *V) const { return LiveVars.count(V) > 0; }
  const ValueSetType &getLiveVars() const { return LiveVars; }
  
private:
  ValueSetType LiveVars;
};

// Liveness analysis (backward dataflow analysis)
class LivenessAnalysis : public DataflowAnalysis<LivenessDomain, Direction::Backward> {
public:
  explicit LivenessAnalysis(llvm::Function &F);
  
  void initialize() override;
  
  // Transfer function for a single instruction
  LivenessDomain transferInstruction(const llvm::Instruction &I, 
                                     const LivenessDomain &State) const override;
  
  // Initial state: empty set of live variables
  LivenessDomain initialState() const override;
  
  // Check if a basic block may affect liveness
  bool mayAffectDomain(const llvm::BasicBlock &BB) const override {
    // A block may affect liveness if it contains any non-debug instructions
    for (const auto &I : BB) {
      if (!I.isDebugOrPseudoInst())
        return true;
    }
    return false;
  }
  
  // Print the live variables for each basic block
  void print(llvm::raw_ostream &OS) const;
};

// Analysis pass for liveness
class LivenessAnalysisPass : public llvm::AnalysisInfoMixin<LivenessAnalysisPass> {
  friend llvm::AnalysisInfoMixin<LivenessAnalysisPass>;
  static llvm::AnalysisKey Key;
  
public:
  using Result = LivenessAnalysis;
  
  LivenessAnalysis run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
};

// Legacy pass for liveness analysis
class LivenessAnalysisWrapperPass : public llvm::FunctionPass {
public:
  static char ID;
  
  LivenessAnalysisWrapperPass();
  
  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  void print(llvm::raw_ostream &OS, const llvm::Module *M = nullptr) const override;
  
  LivenessAnalysis &getResult() { return *Result; }
  const LivenessAnalysis &getResult() const { return *Result; }
  
private:
  std::unique_ptr<LivenessAnalysis> Result;
};

} // end namespace canary

#endif // CANARY_LIVENESS_ANALYSIS_H 