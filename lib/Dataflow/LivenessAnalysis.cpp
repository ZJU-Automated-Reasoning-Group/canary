//===- LivenessAnalysis.cpp - Analysis for live variables ------------------===//
//
// Part of the Canary Project
//
//===----------------------------------------------------------------------===//

#include "Dataflow/LivenessAnalysis.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace canary;

// Initialize static members
char LivenessAnalysisWrapperPass::ID = 0;
AnalysisKey LivenessAnalysisPass::Key;

LivenessAnalysis::LivenessAnalysis(Function &F)
    : DataflowAnalysis<LivenessDomain, Direction::Backward>(F) {
}

void LivenessAnalysis::initialize() {
  // All initialization is done in base class
  DataflowAnalysis::initialize();
}

LivenessDomain LivenessAnalysis::transferInstruction(
    const Instruction &I, const LivenessDomain &State) const {
  LivenessDomain Result = State;

  // Step 1: Kill values defined by this instruction (backward analysis)
  if (!I.isTerminator() && !I.isDebugOrPseudoInst()) {
    Result.removeLiveVar(&I);
  }
  
  // Step 2: Add uses of this instruction (backward analysis)
  for (const Use &U : I.operands()) {
    Value *V = U.get();
    if (isa<Instruction>(V) || isa<Argument>(V)) {
      // Only track instructions and arguments, not constants or globals
      Result.addLiveVar(V);
    }
  }
  
  return Result;
}

LivenessDomain LivenessAnalysis::initialState() const {
  // Initial state for backward liveness analysis: empty
  return LivenessDomain();
}

void LivenessAnalysis::print(raw_ostream &OS) const {
  OS << "Liveness Analysis Results:\n";
  for (auto &BB : F) {
    OS << "BB '" << BB.getName() << "':\n";
    
    // Get liveness information at block entry
    const LivenessDomain BDomain = getResult(&BB);
    
    OS << "  Live variables at entry:";
    if (BDomain.getLiveVars().empty()) {
      OS << " none\n";
    } else {
      OS << "\n";
      for (const Value *V : BDomain.getLiveVars()) {
        OS << "    ";
        if (V->hasName()) {
          OS << V->getName();
        } else {
          V->printAsOperand(OS, false);
        }
        OS << "\n";
      }
    }
    
    // Optionally, we can also compute and print liveness at each instruction
    // For brevity, this is omitted here
  }
}

// Analysis pass implementation
LivenessAnalysis LivenessAnalysisPass::run(Function &F, FunctionAnalysisManager &AM) {
  LivenessAnalysis Analysis(F);
  Analysis.run();
  return Analysis;
}

// Legacy pass implementation
LivenessAnalysisWrapperPass::LivenessAnalysisWrapperPass() 
    : FunctionPass(ID) {}

bool LivenessAnalysisWrapperPass::runOnFunction(Function &F) {
  Result = std::make_unique<LivenessAnalysis>(F);
  Result->run();
  return false; // Analysis pass doesn't modify the function
}

void LivenessAnalysisWrapperPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll(); // Analysis pass doesn't modify the function
}

void LivenessAnalysisWrapperPass::print(raw_ostream &OS, const Module *) const {
  Result->print(OS);
}

// Register the pass
static RegisterPass<LivenessAnalysisWrapperPass>
    X("liveness", "Liveness Analysis", false, true); 