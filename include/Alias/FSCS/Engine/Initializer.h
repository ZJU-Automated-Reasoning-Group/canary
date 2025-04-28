#pragma once

#include "Alias/FSCS/Engine/WorkList.h"
#include "Alias/FSCS/Support/Store.h"

namespace tpa
{

class GlobalState;
class Memo;

class Initializer
{
private:
	GlobalState& globalState;
	Memo& memo;
public:
	Initializer(GlobalState& g, Memo& m): globalState(g), memo(m) {}

	ForwardWorkList runOnInitState(Store&& initStore);
};

}
