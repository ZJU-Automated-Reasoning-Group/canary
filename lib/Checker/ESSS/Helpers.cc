#include <llvm/IR/InstIterator.h>
#include <llvm/Support/Casting.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Operator.h>

#include <utility>
#include <spawn.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include "Checker/ESSS/Common.h"
#include "Checker/ESSS/Helpers.h"
#include "Checker/ESSS/DebugHelpers.h"
#include "Checker/ESSS/ClOptForward.h"


bool isCompositeType(const Type* type) {
    return type->isStructTy()
           || type->isArrayTy()
           || type->isVectorTy();
}

bool canBeUsedInAnIndirectCall(const Function& function) {
    if (!function.hasAddressTaken(nullptr, true, true))
        return false;

    for (const auto* user : function.users()) {
        if (isa<GlobalValue>(user))
            continue;
        if (!all_of(user->user_begin(), user->user_end(), [&](const auto& userUser) {
            return isa<GlobalValue>(userUser) && !isCompositeType(userUser->getType());
        })) {
            return true;
        }
    }

    return false;
}

optional <CalleeMap::const_iterator>
getCalleeIteratorForPotentialCallInstruction(const GlobalContext &Ctx, const Instruction &instruction) {
    // Skip debug instructions since they do not impact the actual functionality.
    if (isa<DbgValueInst>(&instruction) || isa<DbgDeclareInst>(&instruction))
        return {};

    if (auto callInst = dyn_cast<CallInst>(&instruction)) {
        // Includes memory barriers etc
        if (callInst->isInlineAsm())
            return {};

        const auto calleeSetIt = Ctx.Callees.find(callInst);
        if (calleeSetIt != Ctx.Callees.end()) {
            return calleeSetIt;
        }
    }

    return {};
}

optional<vector<string>> getListOfTestCases() {
    if (FunctionTestCasesToAnalyze.empty())
        return optional<vector<string>> {};
    stringstream ss(FunctionTestCasesToAnalyze);
    vector<string> vec;
    string segment;
    while (getline(ss, segment, ',')) {
        vec.emplace_back(std::move(segment));
    }
    return vec;
}

float wilsonScore(float positive, float n, float z) {
    if (n < 0.001f) {
        return 0.0f;
    }

    float phat = positive / n;
    return (phat + z * z / (2.0f * n) + z * sqrtf((phat * (1.0f - phat) + z * z / (4.0f * n)) / n)) / (1.0f + z * z / n);
}

bool isProbablyPure(const Function* function) {
    return function->hasFnAttribute(Attribute::ReadOnly) || function->hasFnAttribute(Attribute::ReadNone) ||
           function->hasFnAttribute(Attribute::Speculatable);
}
