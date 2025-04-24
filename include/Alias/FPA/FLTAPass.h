//
// Created by prophe cheng on 2025/4/1.
//

#ifndef INDIRECTCALLANALYZER_FLTAPASS_H
#define INDIRECTCALLANALYZER_FLTAPASS_H

#include "Alias/FPA/CallGraphPass.h"

class FLTAPass: public CallGraphPass {
public:
    DenseMap<size_t, FuncSet> MatchedFuncsMap;
    DenseMap<size_t, FuncSet> MatchedICallTypeMap;

    FLTAPass(GlobalContext *Ctx_): CallGraphPass(Ctx_){
        ID = "first layer type analysis";
    }

    bool fuzzyTypeMatch(Type *Ty1, Type *Ty2, Module *M1, Module *M2);

    void findCalleesWithType(CallInst*, FuncSet&);

    void analyzeIndCall(CallInst* CI, FuncSet* FS) override;

    bool doInitialization(Module *M) override;
};

#endif //INDIRECTCALLANALYZER_FLTAPASS_H
