#pragma once

#include "Alias/FSCS/FrontEnd/Type/TypeMap.h"
#include "Alias/FSCS/Program/SemiSparseProgram.h"

namespace llvm
{
	class Function;
	class Module;
}

namespace tpa
{

class SemiSparseProgramBuilder
{
private:
	void buildCFGForFunction(tpa::SemiSparseProgram&, const llvm::Function&, const TypeMap&);
public:
	tpa::SemiSparseProgram runOnModule(const llvm::Module& module);
};

}
