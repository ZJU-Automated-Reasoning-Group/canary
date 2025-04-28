
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Debug.h>
#include "Transform/RemoveNoRetFunction.h"

#define DEBUG_TYPE "RemoveNoRetFunction"

char RemoveNoRetFunction::ID = 0;
static RegisterPass<RemoveNoRetFunction> X(DEBUG_TYPE, "removing a function that never returns");

void RemoveNoRetFunction::getAnalysisUsage(AnalysisUsage &AU) const {
}

bool RemoveNoRetFunction::runOnModule(Module &M) {
    for (auto &F: M) {
        if (F.doesNotReturn()) {
            F.deleteBody();
            F.setComdat(nullptr);
        }
    }
    return false;
}
