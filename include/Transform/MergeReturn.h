#ifndef TRANSFORM_MERGERETURN_H
#define TRANSFORM_MERGERETURN_H

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

using namespace llvm;

class MergeReturn : public ModulePass {
public:
    static char ID;

    MergeReturn() : ModulePass(ID) {}

    ~MergeReturn() override = default;

    void getAnalysisUsage(AnalysisUsage &) const override;

    bool runOnModule(Module &) override;
};

#endif //TRANSFORM_MERGERETURN_H
