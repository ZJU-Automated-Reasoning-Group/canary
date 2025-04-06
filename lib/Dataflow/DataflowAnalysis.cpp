//===- DataflowAnalysis.cpp - Framework for dataflow analysis -------------===//
//
// Part of the Canary Project
//
//===----------------------------------------------------------------------===//

#include "Dataflow/DataflowAnalysis.h"

using namespace llvm;
using namespace canary;

// Most of the implementation is header-only through templates

// Explicit include to pull in all headers needed by users of the framework
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/InitializePasses.h"

// Register dataflow analysis passes
namespace {
  // This just ensures passes are properly initialized
  class DataflowAnalysisGroup {
  public:
    DataflowAnalysisGroup() {
      // We'll register passes using the standard LLVM mechanisms
      // Future analyses should call initializeXXXPass
    }
  };
  
  static DataflowAnalysisGroup DataflowAnalysisRegistration;
} 