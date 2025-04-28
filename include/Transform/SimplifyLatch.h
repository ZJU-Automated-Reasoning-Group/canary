#ifndef TRANSFORM_SIMPLIFYLATCH_H
#define TRANSFORM_SIMPLIFYLATCH_H

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

using namespace llvm;

class SimplifyLatch : public ModulePass {
public:
    static char ID;

    SimplifyLatch() : ModulePass(ID) {}

    ~SimplifyLatch() override = default;

    void getAnalysisUsage(AnalysisUsage &) const override;

    bool runOnModule(Module &) override;
};

#endif //TRANSFORM_SIMPLIFYLATCH_H
