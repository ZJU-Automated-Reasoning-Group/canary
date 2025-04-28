#ifndef TRANSFORM_REMOVEDEADBLOCK_H
#define TRANSFORM_REMOVEDEADBLOCK_H

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

using namespace llvm;

class RemoveDeadBlock : public ModulePass {
public:
    static char ID;

    RemoveDeadBlock() : ModulePass(ID) {}

    ~RemoveDeadBlock() override = default;

    void getAnalysisUsage(AnalysisUsage &) const override;

    bool runOnModule(Module &) override;
};

#endif //TRANSFORM_REMOVEDEADBLOCK_H
