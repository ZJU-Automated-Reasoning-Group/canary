#include "Checker/ESSS/DebugHelpers.h"
#include "Checker/ESSS/Helpers.h"
#include "Checker/ESSS/Common.h"

#include <llvm/IR/InstIterator.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Constants.h>

using namespace llvm;
using namespace std;

string getBasicBlockName(const BasicBlock* BB) {
	if (!BB)
		return "nullptr";
	
	if (BB->hasName())
		return BB->getName().str();
	
	return "Block-" + std::to_string((uint64_t)BB);
}

void dumpDebugValue(const Function* function) {
	// Walk through the instructions to find debug info
	for (const auto &B : *function) {
		for (const auto &I : B) {
			const DbgValueInst *DVI = dyn_cast<DbgValueInst>(&I);
			if (DVI) {
				errs() << "Found debug value: ";
				DVI->dump();
			}
			
			const DbgDeclareInst *DDI = dyn_cast<DbgDeclareInst>(&I);
			if (DDI) {
				errs() << "Found debug declare: ";
				DDI->dump();
			}
		}
	}
}

void dumpPathList(esss::span<const BasicBlock* const> blocks) {
	errs() << "Path: ";
	for (const BasicBlock* block : blocks) {
		errs() << getBasicBlockName(block) << " -> ";
	}
	errs() << "end\n";
}
