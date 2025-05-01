#ifndef TRANSFORM_LOWERGLOBALCONSTANTARRAYSELECT_H
#define TRANSFORM_LOWERGLOBALCONSTANTARRAYSELECT_H

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <map>

using namespace llvm;

class LowerGlobalConstantArraySelect : public ModulePass {
private:
    std::map<Value *, Function *> SelectFuncMap;

public:
    static char ID;

    LowerGlobalConstantArraySelect() : ModulePass(ID) {}

    ~LowerGlobalConstantArraySelect() override = default;

    void getAnalysisUsage(AnalysisUsage &) const override;

    bool runOnModule(Module &) override;

private:
    bool isSelectGlobalConstantArray(Instruction &I);

    void initialize(Function *F, ConstantDataArray *CDA);
};

#endif //TRANSFORM_LOWERGLOBALCONSTANTARRAYSELECT_H
