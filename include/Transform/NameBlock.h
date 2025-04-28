#ifndef TRANSFORM_NAMEBLOCK_H
#define TRANSFORM_NAMEBLOCK_H

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

using namespace llvm;

class NameBlock : public ModulePass {
public:
    static char ID;

    NameBlock() : ModulePass(ID) {}

    ~NameBlock() override = default;

    void getAnalysisUsage(AnalysisUsage &) const override;

    bool runOnModule(Module &) override;
};

#endif //TRANSFORM_NAMEBLOCK_H
