#ifndef __DSA_PRINTER_HH_
#define __DSA_PRINTER_HH_

#include "llvm/Pass.h"

namespace seadsa {

/// Pass to print DSA graphs for each function
llvm::Pass *createDsaPrinterPass();

} // namespace seadsa

#endif 