#include "Checker/ESSS/Analyzer.h"
#include "Checker/ESSS/Common.h"
#include "Checker/ESSS/ClOptForward.h"
#include "Checker/ESSS/CallGraph.h"
#include "Checker/ESSS/EHBlockDetector.h"
#include "Checker/ESSS/ErrorCheckViolationFinder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include <mutex>

using namespace llvm;
using namespace std;

// Declaration of external variables defined in Analyzer.cc
extern cl::list<string> InputFilenames;
extern cl::opt<unsigned> VerboseLevel;
extern GlobalContext GlobalCtx;

void loadModule(const string& filename, mutex* modulesVectorMutex) {
    SMDiagnostic Err;
    auto LLVMCtx = new LLVMContext();
    unique_ptr<Module> M = parseIRFile(filename, Err, *LLVMCtx);

    if (!M) {
        errs() << "Error loading file '" << filename << "'\n";
        return;
    }

    Module *Module = M.release();
    StringRef MName(strdup(filename.c_str()));
    lock_guard<mutex> _(*modulesVectorMutex);
    GlobalCtx.Modules.insert({Module, MName});
}

int main(int argc, char **argv) {
    // Print a stack trace if the program crashes
    sys::PrintStackTraceOnErrorSignal(argv[0]);
    PrettyStackTraceProgram X(argc, argv);

    // Initialize only the native target
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    // Parse command line options
    cl::ParseCommandLineOptions(argc, argv, "ESSS - Error Specification and Static Safety-Checker\n");

    // Loading modules
    errs() << "Total " << InputFilenames.size() << " file(s)\n";

    mutex modulesVectorMutex;
    for (unsigned i = 0; i < InputFilenames.size(); ++i) {
        loadModule(InputFilenames[i], &modulesVectorMutex);
    }

    // Run the analysis passes
    {
        CallGraphPass CGPass(&GlobalCtx);
        CGPass.run(GlobalCtx.Modules);
    }

    {
        EHBlockDetectorPass EHPass(&GlobalCtx);
        EHPass.run(GlobalCtx.Modules, true);
        EHPass.nextStage();
        EHPass.associationAnalysisForErrorHandlers();
        EHPass.run(GlobalCtx.Modules, true);
        EHPass.storeData();
        EHPass.learnErrorsFromErrorBlocksForSelf();
        EHPass.propagateCheckedErrors();
    }

    {
        ErrorCheckViolationFinderPass ECVFPass(&GlobalCtx);
        ECVFPass.run(GlobalCtx.Modules);
        ECVFPass.nextStage();
        ECVFPass.run(GlobalCtx.Modules);
        ECVFPass.determineTruncationBugs();
        ECVFPass.determineSignednessBugs();
        ECVFPass.finish();
    }

    // Clean up
    llvm::llvm_shutdown();
    
    return 0;
} 