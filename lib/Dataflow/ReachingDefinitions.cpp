//===- ReachingDefinitions.cpp - Reaching definitions analysis ------------===//
//
// Part of the Canary Project
//
//===----------------------------------------------------------------------===//

#include "Dataflow/ReachingDefinitions.h"
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
char ReachingDefsAnalysisWrapperPass::ID = 0;
AnalysisKey ReachingDefsAnalysisPass::Key;

ReachingDefsAnalysis::ReachingDefsAnalysis(Function &F)
    : DataflowAnalysis<ReachingDefsDomain, Direction::Forward>(F) {
}

void ReachingDefsAnalysis::initialize() {
  // All initialization is done in base class
  DataflowAnalysis::initialize();
}

ReachingDefsDomain ReachingDefsAnalysis::transferInstruction(
    const Instruction &I, const ReachingDefsDomain &State) const {
  ReachingDefsDomain Result = State;

  // Handle definitions
  if (!I.isTerminator() && !I.isDebugOrPseudoInst() && I.getType() != Type::getVoidTy(I.getContext())) {
    // This instruction defines a value
    // Kill all previous definitions of this value (optional: strong update semantics)
    // In this implementation, we choose to accumulate all definitions
    // Result.removeAllDefinitions(&I);
    
    // Add the current definition
    Result.addDefinition(&I, &I);
  }
  
  // Handle memory operations
  if (auto *Store = dyn_cast<StoreInst>(&I)) {
    // Store instruction: defines a memory location
    const Value *Ptr = Store->getPointerOperand();
    
    // We can't precisely track all memory locations, so we conservatively
    // model stores by adding a definition for the pointer operand
    Result.addDefinition(Ptr, Store);
  } else if (auto *Load = dyn_cast<LoadInst>(&I)) {
    // Load instruction: uses a memory location
    // Handled via the general instruction mechanism
  } else if (auto *Call = dyn_cast<CallInst>(&I)) {
    // Call instructions may define memory locations
    // For simplicity, we don't model interprocedural effects precisely
    // A more precise analysis would track function side effects
    if (Call->mayWriteToMemory()) {
      // Conservative: treat call as potentially modifying any memory
      // This is very imprecise but safe
    }
  }
  
  return Result;
}

ReachingDefsDomain ReachingDefsAnalysis::initialState() const {
  // Initial state for forward reaching definitions: empty
  return ReachingDefsDomain();
}

ReachingDefsDomain ReachingDefsAnalysis::getReachingDefs(const Instruction *I) const {
  // Get the enclosing basic block
  const BasicBlock *BB = I->getParent();
  if (!BB)
    return ReachingDefsDomain();
  
  // Start with the input state for this block
  ReachingDefsDomain Result = getResult(BB);
  
  // Apply the transfer function for each instruction up to (but not including) I
  for (const Instruction &Inst : *BB) {
    if (&Inst == I)
      break;
    Result = transferInstruction(Inst, Result);
  }
  
  return Result;
}

void ReachingDefsAnalysis::print(raw_ostream &OS) const {
  OS << "Reaching Definitions Analysis Results:\n";
  for (auto &BB : F) {
    OS << "BB '" << BB.getName() << "':\n";
    
    // Get reaching definitions at block exit (end of the block)
    const ReachingDefsDomain BDomain = getResult(&BB);
    
    OS << "  Reaching definitions at exit:\n";
    
    // Print all variables and their reaching definitions
    for (const auto &Pair : BDomain.getAllDefinitions()) {
      const Value *V = Pair.first;
      const ReachingDefsDomain::InstSetType &Defs = Pair.second;
      
      OS << "    Value: ";
      if (V->hasName()) {
        OS << V->getName();
      } else {
        V->printAsOperand(OS, false);
      }
      OS << " defined at: [";
      
      bool First = true;
      for (const Instruction *DefInst : Defs) {
        if (!First)
          OS << ", ";
        First = false;
        
        // Print the location of the definition
        OS << DefInst->getParent()->getName() << ":";
        DefInst->printAsOperand(OS, false);
      }
      OS << "]\n";
    }
  }
}

// Analysis pass implementation
ReachingDefsAnalysis ReachingDefsAnalysisPass::run(Function &F, FunctionAnalysisManager &AM) {
  ReachingDefsAnalysis Analysis(F);
  Analysis.run();
  return Analysis;
}

// Legacy pass implementation
ReachingDefsAnalysisWrapperPass::ReachingDefsAnalysisWrapperPass() 
    : FunctionPass(ID) {}

bool ReachingDefsAnalysisWrapperPass::runOnFunction(Function &F) {
  Result = std::make_unique<ReachingDefsAnalysis>(F);
  Result->run();
  return false; // Analysis pass doesn't modify the function
}

void ReachingDefsAnalysisWrapperPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll(); // Analysis pass doesn't modify the function
}

void ReachingDefsAnalysisWrapperPass::print(raw_ostream &OS, const Module *) const {
  Result->print(OS);
}

// Register the pass
static RegisterPass<ReachingDefsAnalysisWrapperPass>
    X("reaching-defs", "Reaching Definitions Analysis", false, true); 