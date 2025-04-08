#include "Alias/Andersen/Andersen.h"
#include "Alias/Andersen/AndersenAA.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

static cl::opt<std::string> InputFilename(cl::Positional,
                                        cl::desc("<IR file>"),
                                        cl::Required);

int main(int argc, char **argv) {
  // Parse command line arguments
  cl::ParseCommandLineOptions(argc, argv, "Andersen Pointer Analysis Example\n");

  // Create context and load module
  LLVMContext Context;
  SMDiagnostic Err;
  
  std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
  if (!M) {
    Err.print(argv[0], errs());
    return 1;
  }

  errs() << "Running Andersen's analysis on " << InputFilename << "\n";
  
  // Run Andersen analysis
  Andersen anders(*M);

  // Collect all allocation sites
  std::vector<const Value *> allocSites;
  anders.getAllAllocationSites(allocSites);
  
  errs() << "Allocation sites found: " << allocSites.size() << "\n";
  for (const Value *V : allocSites) {
    errs() << "  ";
    V->print(errs());
    errs() << "\n";
  }

  // Print points-to information for each global variable
  errs() << "\nPoints-to information for global variables:\n";
  for (const GlobalVariable &GV : M->globals()) {
    if (!GV.getType()->isPointerTy())
      continue;
      
    std::vector<const Value *> ptsSet;
    bool success = anders.getPointsToSet(&GV, ptsSet);
    
    errs() << GV.getName() << " points to ";
    if (!success) {
      errs() << "unknown (possibly universal set)\n";
      continue;
    }
    
    if (ptsSet.empty()) {
      errs() << "nothing (empty set)\n";
      continue;
    }
    
    errs() << ptsSet.size() << " location(s):\n";
    for (const Value *V : ptsSet) {
      errs() << "  ";
      V->print(errs());
      errs() << "\n";
    }
  }
  
  return 0;
} 