#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Debug.h>
#include "Transform/NameBlock.h"

#define DEBUG_TYPE "NameBlock"

char NameBlock::ID = 0;
static RegisterPass<NameBlock> X(DEBUG_TYPE, "Naming each block for dbg");

void NameBlock::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
}

bool NameBlock::runOnModule(Module &M) {
    for (auto &F: M) {
        unsigned BI = 0;
        for (auto &B: F) {
            if (!B.hasName()) B.setName("B" + std::to_string(++BI));
        }
    }
    return false;
}