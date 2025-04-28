#ifndef TRANSFORM_REMOVENORETFUNCTION_H
#define TRANSFORM_REMOVENORETFUNCTION_H

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

using namespace llvm;

class RemoveNoRetFunction : public ModulePass {
public:
    static char ID;

    RemoveNoRetFunction() : ModulePass(ID) {}

    ~RemoveNoRetFunction() override = default;

    void getAnalysisUsage(AnalysisUsage &) const override;

    bool runOnModule(Module &) override;
};

#endif //TRANSFORM_REMOVENORETFUNCTION_H
