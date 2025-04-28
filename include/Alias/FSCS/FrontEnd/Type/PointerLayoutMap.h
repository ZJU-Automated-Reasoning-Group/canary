#pragma once

#include "Alias/FSCS/FrontEnd/ConstPointerMap.h"

namespace llvm
{
	class Type;
}

namespace tpa
{

class PointerLayout;

using PointerLayoutMap = ConstPointerMap<llvm::Type, PointerLayout>;

}
