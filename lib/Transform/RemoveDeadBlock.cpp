
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Debug.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "Transform/RemoveDeadBlock.h"

#define DEBUG_TYPE "RemoveDeadBlock"

char RemoveDeadBlock::ID = 0;
static RegisterPass<RemoveDeadBlock> X(DEBUG_TYPE, "remove dead block, not sure why simplifycfg does not work");

void RemoveDeadBlock::getAnalysisUsage(AnalysisUsage &AU) const {
}

bool RemoveDeadBlock::runOnModule(Module &M) {
    std::vector<BasicBlock *> Block2Remove;
    std::set<BasicBlock *> Visited;
    std::set<BasicBlock *> SuccVec;

    for (auto &F: M) {
        Block2Remove.clear();
        for (auto &B: F) {
            if (pred_size(&B) == 0 && (&B != &F.getEntryBlock() || B.getSinglePredecessor() == &B)) {
                Block2Remove.push_back(&B);
            }
        }

        // do transitively remove
        Visited.clear();
        while (!Block2Remove.empty()) {
            auto *B = Block2Remove.back();
            Block2Remove.pop_back();
            if (Visited.count(B)) continue;
            Visited.insert(B);

            SuccVec.clear();
            for (auto It = succ_begin(B), E = succ_end(B); It != E; ++It) {
                SuccVec.insert(*It);
            }
            DeleteDeadBlock(B);

            for (auto *Succ : SuccVec) {
                if (pred_size(Succ) == 0 && (Succ != &F.getEntryBlock() || Succ->getSinglePredecessor() == Succ)) {
                    Block2Remove.push_back(Succ);
                }
            }
        }
    }

    if (verifyModule(M, &errs())) {
        llvm_unreachable("Error: RemoveDeadBlock fails...");
    }
    return false;
}