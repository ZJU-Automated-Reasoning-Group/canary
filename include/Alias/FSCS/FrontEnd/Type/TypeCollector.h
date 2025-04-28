#pragma once

#include "Alias/FSCS/FrontEnd/Type/TypeSet.h"

namespace llvm
{
	class Module;
}

namespace tpa
{

class TypeCollector
{
public:
	TypeCollector() = default;

	TypeSet runOnModule(const llvm::Module&);
};

}
