#ifndef __DSA_INITIALIZE_PASSES_HH_
#define __DSA_INITIALIZE_PASSES_HH_

#include "llvm/PassRegistry.h"

namespace seadsa {

void initializeAnalysisPasses(llvm::PassRegistry &Registry);

} // namespace seadsa

#endif
