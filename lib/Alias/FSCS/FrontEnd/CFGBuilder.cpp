#include "Alias/FSCS/FrontEnd/CFG/CFGBuilder.h"
#include "Alias/FSCS/FrontEnd/CFG/CFGSimplifier.h"
#include "Alias/FSCS/FrontEnd/CFG/FunctionTranslator.h"
#include "Alias/FSCS/FrontEnd/CFG/InstructionTranslator.h"
#include "Alias/FSCS/Program/CFG/CFG.h"

#include <llvm/IR/DataLayout.h>

using namespace llvm;

namespace tpa
{

CFGBuilder::CFGBuilder(CFG& c, const TypeMap& t): cfg(c), typeMap(t)
{

}

void CFGBuilder::buildCFG(const Function& llvmFunc)
{
	auto dataLayout = DataLayout(llvmFunc.getParent());
	auto instTranslator = InstructionTranslator(cfg, typeMap, dataLayout);
	FunctionTranslator(cfg, instTranslator).translateFunction(llvmFunc);
	CFGSimplifier().simplify(cfg);
	cfg.buildValueMap();
}

}