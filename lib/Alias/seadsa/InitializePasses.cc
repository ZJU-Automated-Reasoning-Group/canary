#include "Alias/seadsa/InitializePasses.hh"

#include "llvm/PassRegistry.h"
#include "llvm/InitializePasses.h"

namespace seadsa {

void initializeAnalysisPasses(llvm::PassRegistry &Registry) {
  // Initialize LLVM passes
  llvm::initializeTargetLibraryInfoWrapperPassPass(Registry);
  llvm::initializeCallGraphWrapperPassPass(Registry);
  llvm::initializeDominatorTreeWrapperPassPass(Registry);
  llvm::initializeLoopInfoWrapperPassPass(Registry);
}

} // namespace seadsa 