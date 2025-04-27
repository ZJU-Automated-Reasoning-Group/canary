#ifndef LOTUS_POINTER_ANALYSIS_INTERFACE_H
#define LOTUS_POINTER_ANALYSIS_INTERFACE_H

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Pass.h"

#include <vector>
#include <set>
#include <memory>
#include <string>

namespace lotus {

/// \brief Interface for pointer analysis results
///
/// This class defines the interface for accessing different pointer analyses
/// in a unified way. It's designed to be used by clients that need alias information
/// but don't care about the specific implementation details.
class PointerAnalysisResult {
public:
  virtual ~PointerAnalysisResult() {}

  /// Query whether two memory locations may alias, must alias, or do not alias
  virtual llvm::AliasResult alias(const llvm::MemoryLocation &LocA,
                                 const llvm::MemoryLocation &LocB) = 0;

  /// Simpler overload for querying two Values directly
  llvm::AliasResult alias(const llvm::Value *V1, const llvm::Value *V2) {
    return alias(llvm::MemoryLocation::getBeforeOrAfter(V1), 
                 llvm::MemoryLocation::getBeforeOrAfter(V2));
  }

  /// Get the points-to set for a pointer
  virtual std::vector<const llvm::Value*> getPointsToSet(const llvm::Value *Ptr) = 0;

  /// Check if Ptr may point to Target
  virtual bool pointsTo(const llvm::Value *Ptr, const llvm::Value *Target) = 0;
  
  /// Check if Val is a constant or points to constant memory
  virtual bool pointsToConstantMemory(const llvm::MemoryLocation &Loc, bool OrLocal = false) = 0;
};

/// \brief Wrapper class for accessing Andersen's pointer analysis
class AndersenPointerAnalysisResult : public PointerAnalysisResult {
private:
  class Implementation;
  std::unique_ptr<Implementation> Impl;

public:
  AndersenPointerAnalysisResult(const llvm::Module &M);
  ~AndersenPointerAnalysisResult() override;

  llvm::AliasResult alias(const llvm::MemoryLocation &LocA,
                         const llvm::MemoryLocation &LocB) override;
  std::vector<const llvm::Value*> getPointsToSet(const llvm::Value *Ptr) override;
  bool pointsTo(const llvm::Value *Ptr, const llvm::Value *Target) override;
  bool pointsToConstantMemory(const llvm::MemoryLocation &Loc, bool OrLocal = false) override;
};

/// \brief Wrapper class for accessing CFLAnder pointer analysis
class CFLAnderPointerAnalysisResult : public PointerAnalysisResult {
private:
  class Implementation;
  std::unique_ptr<Implementation> Impl;

public:
  CFLAnderPointerAnalysisResult(const llvm::Module &M);
  ~CFLAnderPointerAnalysisResult() override;

  llvm::AliasResult alias(const llvm::MemoryLocation &LocA,
                         const llvm::MemoryLocation &LocB) override;
  std::vector<const llvm::Value*> getPointsToSet(const llvm::Value *Ptr) override;
  bool pointsTo(const llvm::Value *Ptr, const llvm::Value *Target) override;
  bool pointsToConstantMemory(const llvm::MemoryLocation &Loc, bool OrLocal = false) override;
};

/// \brief Wrapper class for accessing CFLSteens pointer analysis
class CFLSteensPointerAnalysisResult : public PointerAnalysisResult {
private:
  class Implementation;
  std::unique_ptr<Implementation> Impl;

public:
  CFLSteensPointerAnalysisResult(const llvm::Module &M);
  ~CFLSteensPointerAnalysisResult() override;

  llvm::AliasResult alias(const llvm::MemoryLocation &LocA,
                         const llvm::MemoryLocation &LocB) override;
  std::vector<const llvm::Value*> getPointsToSet(const llvm::Value *Ptr) override;
  bool pointsTo(const llvm::Value *Ptr, const llvm::Value *Target) override;
  bool pointsToConstantMemory(const llvm::MemoryLocation &Loc, bool OrLocal = false) override;
};

/// \brief Wrapper class for accessing DyckAA pointer analysis
class DyckAAPointerAnalysisResult : public PointerAnalysisResult {
private:
  class Implementation;
  std::unique_ptr<Implementation> Impl;

public:
  DyckAAPointerAnalysisResult(const llvm::Module &M);
  ~DyckAAPointerAnalysisResult() override;

  llvm::AliasResult alias(const llvm::MemoryLocation &LocA,
                         const llvm::MemoryLocation &LocB) override;
  std::vector<const llvm::Value*> getPointsToSet(const llvm::Value *Ptr) override;
  bool pointsTo(const llvm::Value *Ptr, const llvm::Value *Target) override;
  bool pointsToConstantMemory(const llvm::MemoryLocation &Loc, bool OrLocal = false) override;
};

/// \brief Factory for creating pointer analysis result objects
class PointerAnalysisFactory {
public:
  /// Create a pointer analysis result object based on the specified type
  /// \param M The module to analyze
  /// \param Type The type of pointer analysis to use ("andersen", "cfl-anders", "cfl-steens", "dyck", etc.)
  static std::unique_ptr<PointerAnalysisResult> create(const llvm::Module &M, 
                                                       const std::string &Type);
};

/// \brief ModulePass wrapper for pointer analysis
///
/// This pass allows clients to access pointer analysis results through LLVM's
/// pass manager infrastructure.
class PointerAnalysisWrapperPass : public llvm::ModulePass {
private:
  std::string AnalysisType;
  std::unique_ptr<PointerAnalysisResult> Result;

public:
  static char ID;
  
  explicit PointerAnalysisWrapperPass(const std::string &Type = "andersen");
  ~PointerAnalysisWrapperPass() override;

  bool runOnModule(llvm::Module &M) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  PointerAnalysisResult &getResult() { return *Result; }
  const PointerAnalysisResult &getResult() const { return *Result; }
};

} // end namespace lotus

#endif // LOTUS_POINTER_ANALYSIS_INTERFACE_H 