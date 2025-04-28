#ifndef TRANSFORM_LOWERSELECT_H
#define TRANSFORM_LOWERSELECT_H

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

using namespace llvm;

class LowerSelect : public ModulePass {
public:
    static char ID;

    LowerSelect() : ModulePass(ID) {}

    ~LowerSelect() override = default;

    void getAnalysisUsage(AnalysisUsage &) const override;

    bool runOnModule(Module &) override;
};

#endif //TRANSFORM_LOWERSELECT_H
