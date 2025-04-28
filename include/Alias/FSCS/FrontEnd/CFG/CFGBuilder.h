#pragma once

#include "Alias/FSCS/FrontEnd/Type/TypeMap.h"

namespace llvm
{
	class Function;
	class Instruction;
}

namespace tpa
{

class CFG;
class CFGNode;

class CFGBuilder
{
private:
	CFG& cfg;
	const TypeMap& typeMap;

	CFGNode* translateInstruction(const llvm::Instruction&);
public:
	CFGBuilder(CFG& c, const TypeMap& t);

	void buildCFG(const llvm::Function&);
};

}
