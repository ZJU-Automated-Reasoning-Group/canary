
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Debug.h>
#include "Transform/SimplifyLatch.h"

#define DEBUG_TYPE "SimplifyLatch"

char SimplifyLatch::ID = 0;
static RegisterPass<SimplifyLatch> X(DEBUG_TYPE, "Make latch unconditional");

void SimplifyLatch::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<LoopInfoWrapperPass>();
}

static void transform(BasicBlock *Latch, unsigned ToHeader) {
    auto *NewLatch = BasicBlock::Create(Latch->getContext(), "", Latch->getParent(), Latch);
    auto *Term = Latch->getTerminator();
    auto *Header = Term->getSuccessor(ToHeader);
    Term->setSuccessor(ToHeader, NewLatch);
    BranchInst::Create(Header, NewLatch);

    for (auto &P: *Header) {
        if (auto *Phi = dyn_cast<PHINode>(&P)) {
            for (unsigned K = 0; K < Phi->getNumIncomingValues(); ++K) {
                if (Phi->getIncomingBlock(K) == Latch) {
                    Phi->setIncomingBlock(K, NewLatch);
                }
            }
        } else {
            break;
        }
    }
}

bool SimplifyLatch::runOnModule(Module &M) {
    std::vector<std::pair<BasicBlock *, unsigned>> LatchVector;
    for (auto &F: M) {
        if (F.empty()) continue;
        auto LI = &getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
        auto AllLoops = LI->getLoopsInPreorder();
        for (auto *Loop: AllLoops) {
            auto *Latch = Loop->getLoopLatch();
            auto Term = Latch->getTerminator();
            if (Term->getNumSuccessors() > 1) {
                for (unsigned K = 0; K < Term->getNumSuccessors(); ++K) {
                    if (Term->getSuccessor(K) == Loop->getHeader()) {
                        LatchVector.emplace_back(Loop->getLoopLatch(), K);
                        break;
                    }
                }
            }
        }
    }

    for (auto &It: LatchVector) {
        transform(It.first, It.second);
    }

    if (verifyModule(M, &errs())) {
        llvm_unreachable("Error: SimplifyLatch fails...");
    }
    return false;
}