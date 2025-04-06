//===- DataflowAnalysis.h - Framework for dataflow analysis -------*- C++ -*-===//
//
// Part of the Canary Project
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines the interface for the dataflow analysis framework.
///
//===----------------------------------------------------------------------===//

#ifndef CANARY_DATAFLOW_ANALYSIS_H
#define CANARY_DATAFLOW_ANALYSIS_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"

namespace canary {

// Template for abstract domain in dataflow analysis
template <typename T>
class AbstractDomain {
public:
  virtual ~AbstractDomain() = default;

  // Meet operation (intersection for forward may analysis, union for backwards must analysis)
  virtual T meet(const T &Other) const = 0;

  // Whether this domain equals another
  virtual bool equals(const T &Other) const = 0;
};

// Direction of dataflow analysis
enum class Direction {
  Forward,  // Analyze from entry to exit
  Backward  // Analyze from exit to entry
};

// Generic dataflow analysis framework
template <typename DomainT, Direction Dir>
class DataflowAnalysis {
public:
  using DomainMapTy = llvm::DenseMap<const llvm::BasicBlock *, DomainT>;

  explicit DataflowAnalysis(llvm::Function &F) : F(F) {
    // Defer initialization to initialize() method
  }
  
  virtual ~DataflowAnalysis() = default;

  // Initialize the analysis data structures
  virtual void initialize() {
    // Initialize empty states for all basic blocks
    for (auto &BB : F) {
      BBStates[&BB] = initialState();
    }
  }

  // Perform the actual analysis
  void run() {
    // Make sure we're initialized
    if (BBStates.empty()) {
      initialize();
    }
    
    // Initialize worklist with all basic blocks
    llvm::SetVector<const llvm::BasicBlock *> worklist;
    for (auto &BB : F) {
      worklist.insert(&BB);
    }
    
    // Process blocks until worklist is empty
    while (!worklist.empty()) {
      // Get a block from the worklist
      const llvm::BasicBlock *BB = worklist.pop_back_val();
      
      // Get input state for this block
      DomainT input = getBlockInput(*BB);
      
      // Propagate through the block
      DomainT output = transferBlock(*BB, input);
      
      // Check if output state changed
      if (!output.equals(BBStates[BB])) {
        BBStates[BB] = output;
        
        // Add successors/predecessors to worklist
        if (Dir == Direction::Forward) {
          for (const llvm::BasicBlock *succ : llvm::successors(BB)) {
            worklist.insert(succ);
          }
        } else {
          for (const llvm::BasicBlock *pred : llvm::predecessors(BB)) {
            worklist.insert(pred);
          }
        }
      }
    }
  }

  // Get the input state for a basic block
  DomainT getBlockInput(const llvm::BasicBlock &BB) const {
    bool isEntry = &BB == &F.getEntryBlock();
    
    if (Dir == Direction::Forward) {
      if (isEntry) {
        // For entry block, use initial state
        return initialState();
      } else {
        // For other blocks, merge incoming states
        // Count predecessors first to optimize common cases
        const unsigned numPreds = std::distance(llvm::pred_begin(&BB), llvm::pred_end(&BB));
        
        if (numPreds == 0) {
          // Unreachable block - use initial state
          return initialState();
        } else if (numPreds == 1) {
          // Single predecessor case - just return its state
          const llvm::BasicBlock *pred = *llvm::pred_begin(&BB);
          auto it = BBStates.find(pred);
          if (it != BBStates.end()) {
            return it->second;
          }
          return initialState();
        } else {
          // Multiple predecessors - use meet operation
          DomainT result;
          bool first = true;
          
          for (const auto *pred : llvm::predecessors(&BB)) {
            auto it = BBStates.find(pred);
            if (it != BBStates.end()) {
              if (first) {
                result = it->second;
                first = false;
              } else {
                result = result.meet(it->second);
              }
            }
          }
          
          return result;
        }
      }
    } else { // Backward
      const unsigned numSuccs = BB.getTerminator()->getNumSuccessors();
      
      if (numSuccs == 0) {
        // For exit blocks, use initial state
        return initialState();
      } else if (numSuccs == 1) {
        // Single successor case - just return its state
        const llvm::BasicBlock *succ = BB.getTerminator()->getSuccessor(0);
        auto it = BBStates.find(succ);
        if (it != BBStates.end()) {
          return it->second;
        }
        return initialState();
      } else {
        // Multiple successors - use meet operation
        DomainT result;
        bool first = true;
        
        for (const auto *succ : llvm::successors(&BB)) {
          auto it = BBStates.find(succ);
          if (it != BBStates.end()) {
            if (first) {
              result = it->second;
              first = false;
            } else {
              result = result.meet(it->second);
            }
          }
        }
        
        return result;
      }
    }
  }

  // Apply transfer function to a basic block
  virtual DomainT transferBlock(const llvm::BasicBlock &BB, const DomainT &Input) const {
    // Fast path: if the block is empty, just return the input
    if (BB.empty())
      return Input;
      
    DomainT state = Input;
    
    // Skip transfer if there's only a single non-terminator instruction
    // that doesn't affect our domain (common case optimization)
    if (BB.size() <= 2 && !mayAffectDomain(BB))
      return state;
    
    if (Dir == Direction::Forward) {
      // Process instructions in forward order
      for (const auto &I : BB) {
        state = transferInstruction(I, state);
      }
    } else {
      // Process instructions in reverse order
      for (auto I = BB.rbegin(), E = BB.rend(); I != E; ++I) {
        state = transferInstruction(*I, state);
      }
    }
    
    return state;
  }

  // Predicate to determine if a basic block may affect our domain
  virtual bool mayAffectDomain(const llvm::BasicBlock &BB) const {
    return true; // Conservative default: assume all blocks may affect the domain
  }

  // Apply transfer function to a single instruction
  virtual DomainT transferInstruction(const llvm::Instruction &I, const DomainT &State) const = 0;

  // Get initial dataflow value
  virtual DomainT initialState() const = 0;

  // Get results for a specific basic block
  DomainT getResult(const llvm::BasicBlock *BB) const {
    auto it = BBStates.find(BB);
    if (it != BBStates.end()) {
      return it->second;
    }
    return DomainT();  // Return default domain if not found
  }

protected:
  llvm::Function &F;
  DomainMapTy BBStates;
};

} // end namespace canary

#endif // CANARY_DATAFLOW_ANALYSIS_H 