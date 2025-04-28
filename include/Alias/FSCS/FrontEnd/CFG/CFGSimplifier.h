#pragma once

#include "Support/ADT/VectorSet.h"

#include <llvm/ADT/DenseMap.h>

namespace llvm
{
	class Value;
}

namespace tpa
{

class CFG;
class CFGNode;

class CFGSimplifier
{
private:
	llvm::DenseMap<const llvm::Value*, const llvm::Value*> eqMap;

	std::vector<CFGNode*> findRedundantNodes(CFG&);
	void flattenEquivalentMap();
	void adjustCFG(CFG&, const util::VectorSet<CFGNode*>&);
	void adjustDefUseChain(const util::VectorSet<CFGNode*>&);
	void removeNodes(CFG&, const util::VectorSet<CFGNode*>&);
public:
	CFGSimplifier() {}

	void simplify(CFG&);
};

}