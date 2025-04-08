#include <llvm/IR/InstIterator.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Constants.h>

#include "Checker/ESSS/Helpers.h"
#include "Checker/ESSS/DebugHelpers.h"
#include "Checker/ESSS/Common.h"

using namespace std;

string getBasicBlockName(const BasicBlock* BB) {
	string str;
	raw_string_ostream rso(str);
    rso << BB->getParent()->getName() << ':';
	BB->printAsOperand(rso, false);
	return rso.str();
}

void dumpDebugValue(const Function* function) {
    for (const auto& instruction : instructions(function)) {
		if (auto dbg = dyn_cast<DbgDeclareInst>(&instruction)) {
			// dbg->getAddress()->getType()->dump();
			// dbg->getAddress()->dump();
		} else if (auto dbg = dyn_cast<DbgValueInst>(&instruction)) {
			// dbg->getValue()->getType()->dump();
			// dbg->getValue()->dump();
		}
	}
}

void dumpPathList(span<const BasicBlock* const> blocks) {
    LOG(LOG_INFO, "Path of length " << blocks.size() << ": ");
    for (const auto* block : blocks) {
#if 0
        LOG(LOG_INFO, "(" << block << ") ");
#endif
        LOG(LOG_INFO, getBasicBlockName(block) << " -> ");
    }
    LOG(LOG_INFO, "$\n");
}
