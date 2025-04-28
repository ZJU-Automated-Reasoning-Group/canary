#pragma once

#include "Alias/FSCS/FrontEnd/Type/TypeMap.h"

namespace llvm
{
	class Module;
}

namespace tpa
{

class TypeAnalysis
{
public:
	TypeAnalysis() = default;

	TypeMap runOnModule(const llvm::Module&);
};

}