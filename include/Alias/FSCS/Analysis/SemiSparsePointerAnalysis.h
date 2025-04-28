#pragma once

#include "Alias/FSCS/Analysis/PointerAnalysis.h"
#include "Alias/FSCS/Analysis/PointerAnalysisQueries.h"
#include "Alias/FSCS/Support/Env.h"
#include "Alias/FSCS/Support/Memo.h"

#include <memory>

namespace tpa
{

class SemiSparseProgram;

class SemiSparsePointerAnalysis: public PointerAnalysis<SemiSparsePointerAnalysis>
{
private:
	Env env;
	Memo memo;
public:
	SemiSparsePointerAnalysis() = default;

	void runOnProgram(const SemiSparseProgram&);

	PtsSet getPtsSetImpl(const Pointer*) const;
	
	// Get a non-const reference to the pointer manager
	PointerManager& getMutablePointerManager() { return ptrManager; }
	
	// Get a query interface for this pointer analysis
	std::unique_ptr<PointerAnalysisQueries> createQueryInterface() const
	{
		return std::make_unique<PointerAnalysisQueriesImpl<SemiSparsePointerAnalysis>>(*this);
	}
};

}
