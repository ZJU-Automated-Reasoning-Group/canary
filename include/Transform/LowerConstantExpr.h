#ifndef TRANSFORM_LOWERCONSTANTEXPR_H
#define TRANSFORM_LOWERCONSTANTEXPR_H

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

using namespace llvm;

class LowerConstantExpr : public ModulePass {
public:
    static char ID;

    LowerConstantExpr() : ModulePass(ID) {}

    ~LowerConstantExpr() override = default;

    void getAnalysisUsage(AnalysisUsage &) const override;

    bool runOnModule(Module &) override;
};

#endif //TRANSFORM_LOWERCONSTANTEXPR_H
