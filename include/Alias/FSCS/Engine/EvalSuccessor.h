#pragma once

#include "Alias/FSCS/Context/ProgramPoint.h"
#include "Alias/FSCS/Support/Store.h"

namespace tpa
{

class EvalSuccessor
{
private:
	context::ProgramPoint pp;
	const Store* store;

	EvalSuccessor(const context::ProgramPoint& p, const Store* s): pp(p), store(s) {}
public:
	bool isTopLevel() const { return store == nullptr; }
	const context::ProgramPoint& getProgramPoint() const { return pp; }
	const Store* getStore() const { return store; }

	friend class EvalResult;
};

}
