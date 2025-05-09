#include "Alias/Dynamic/FeatureCheck.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace dynamic {

void FeatureCheck::issueWarning(const Value& v, const StringRef& msg) {
    errs().changeColor(raw_ostream::RED);
    errs() << v.getName() << ": " << msg << "\n";
    errs().resetColor();
}

void FeatureCheck::checkIndirectLibraryCall(const Function& f) {
    if (f.isDeclaration()) {
        for (auto user : f.users()) {
            // In LLVM 14, CallSite was removed. We need to check for CallInst and InvokeInst
            bool isCall = isa<CallInst>(user) || isa<InvokeInst>(user);
            if (!isCall)
                continue;
            
            // Process CallInst
            if (auto *callInst = dyn_cast<CallInst>(user)) {
                for (unsigned i = 0; i < callInst->arg_size(); ++i)
                    if (callInst->getArgOperand(i) == &f)
                        issueWarning(f, "potential indirect external call found!");
            }
            // Process InvokeInst
            else if (auto *invokeInst = dyn_cast<InvokeInst>(user)) {
                for (unsigned i = 0; i < invokeInst->arg_size(); ++i)
                    if (invokeInst->getArgOperand(i) == &f)
                        issueWarning(f, "potential indirect external call found!");
            }
        }
    }
}

void FeatureCheck::checkArrayArgOrInst(const Function& f) {
    for (auto const& arg : f.args())
        if (arg.getType()->isArrayTy())
            issueWarning(arg, "param is an array");

    for (auto const& bb : f)
        for (auto const& inst : bb)
            if (inst.getType()->isArrayTy())
                issueWarning(inst, "inst is an array");
}

void FeatureCheck::runOnModule(const Module& module) {
    for (auto const& f : module) {
        checkIndirectLibraryCall(f);
        checkArrayArgOrInst(f);
    }
}
}
