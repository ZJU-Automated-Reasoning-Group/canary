#pragma once

#include "Alias/FSCS/FrontEnd/ConstPointerMap.h"

namespace llvm
{
	class Type;
}

namespace tpa
{

class ArrayLayout;

using ArrayLayoutMap = ConstPointerMap<llvm::Type, ArrayLayout>;

}
