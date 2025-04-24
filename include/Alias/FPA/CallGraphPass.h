//
// Created by prophe cheng on 2024/1/3.
//

#ifndef TYPEDIVE_ANALYZER_H
#define TYPEDIVE_ANALYZER_H

#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Support/CommandLine.h>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "Alias/FPA/Common.h"

// 主要关注3个函数，doInitialization、doFinalization、doModulePass
// doInitialization完成type-hierachy分析以及type-propagation
// doFinalization将结果dump出
// doModulePass进行间接调用分析
class CallGraphPass {
public:
    const char* ID;
    GlobalContext* Ctx;
    // General pointer types like char * and void *
    map<Module*, Type*> Int8PtrTy;
    // long interger type
    map<Module*, Type*> IntPtrTy;
    map<Module*, const DataLayout*> DLMap;
    set<CallInst*> CallSet;
    set<CallInst*> ICallSet;
    set<CallInst*> VCallSet;
    set<CallInst*> MatchedICallSet;

    int MIdx;

    CallGraphPass(GlobalContext *Ctx_, const char *ID_ = NULL): Ctx(Ctx_), ID(ID_), MIdx(0) { }

    // Run on each module before iterative pass.
    virtual bool doInitialization(Module *M);

    // Run on each module after iterative pass.
    virtual bool doFinalization(Module *M);

    // Iterative pass.
    virtual bool doModulePass(Module *M);

    virtual void analyzeIndCall(CallInst* callInst, FuncSet* FS) = 0;

    void run(ModuleList &modules);

    void intersectFuncSets(FuncSet &FS1, FuncSet &FS2, FuncSet &FS);

    void unrollLoops(Function* F);

    bool isVirtualCall(CallInst* CI);

    bool isVirtualFunction(Function* F);
};

#endif //TYPEDIVE_ANALYZER_H
