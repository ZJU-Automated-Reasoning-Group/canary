#pragma once

#include "Alias/FSCS/Context/ProgramPoint.h"
#include "Alias/FSCS/Support/Env.h"
#include "Alias/FSCS/Support/Store.h"
#include "Support/ADT/VectorSet.h"

#include <unordered_map>
#include <unordered_set>

namespace tpa
{

class MemoryManager;
class PointerManager;

class StorePruner
{
private:
	const Env& env;
	const PointerManager& ptrManager;
	const MemoryManager& memManager;

	using ObjectSet = std::unordered_set<const MemoryObject*>;
	ObjectSet getRootSet(const Store&, const context::ProgramPoint&);
	void findAllReachableObjects(const Store&, ObjectSet&);
	Store filterStore(const Store&, const ObjectSet&);
public:
	StorePruner(const Env& e, const PointerManager& p, const MemoryManager& m): env(e), ptrManager(p), memManager(m) {}

	Store pruneStore(const Store&, const context::ProgramPoint&);
};

}
