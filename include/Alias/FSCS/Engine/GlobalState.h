#pragma once

#include "Alias/FSCS/Support/CallGraph.h"
#include "Alias/FSCS/Support/Env.h"
#include "Alias/FSCS/Support/FunctionContext.h"
#include "Alias/FSCS/Context/ProgramPoint.h"

namespace annotation
{
	class ExternalPointerTable;
}

namespace tpa
{

class MemoryManager;
class PointerManager;
class SemiSparseProgram;

class GlobalState
{
private:
	PointerManager& ptrManager;
	MemoryManager& memManager;

	const SemiSparseProgram& prog;
	const annotation::ExternalPointerTable& extTable;

	Env& env;
	CallGraph<context::ProgramPoint, FunctionContext> callGraph;
public:
	GlobalState(PointerManager& p, MemoryManager& m, const SemiSparseProgram& s, const annotation::ExternalPointerTable& t, Env& e): ptrManager(p), memManager(m), prog(s), extTable(t), env(e) {}

	PointerManager& getPointerManager() { return ptrManager; }
	const PointerManager& getPointerManager() const { return ptrManager; }
	MemoryManager& getMemoryManager() { return memManager; }
	const MemoryManager& getMemoryManager() const { return memManager; }
	const SemiSparseProgram& getSemiSparseProgram() const { return prog; }
	const annotation::ExternalPointerTable& getExternalPointerTable() const { return extTable; }

	Env& getEnv() { return env; }
	const Env& getEnv() const { return env; }

	decltype(callGraph)& getCallGraph() { return callGraph; }
	const decltype(callGraph)& getCallGraph() const { return callGraph; }
};

}
