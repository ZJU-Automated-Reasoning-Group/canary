#include "Alias/FSCS/FrontEnd/Type/StructCastAnalysis.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>

using namespace llvm;

namespace llvm
{

/**
 * @brief Helper class to access bitcast operations
 * 
 * This class extends LLVM's Operator infrastructure to provide
 * convenient access to bitcast operations, regardless of whether
 * they appear as instructions or constant expressions.
 */
class BitCastOperator: public ConcreteOperator<Operator, Instruction::BitCast>
{
	friend class BitCastInst;
	friend class ConstantExpr;
public:
	/**
	 * @brief Gets the source type of the bitcast
	 * @return The type being cast from
	 */
	Type* getSrcTy() const
	{
		return getOperand(0)->getType();
	}

	/**
	 * @brief Gets the destination type of the bitcast
	 * @return The type being cast to
	 */
	Type* getDestTy() const
	{
		return getType();
	}
};

}

namespace tpa
{

namespace
{

/**
 * @brief Helper class to build a map of structure type casts
 * 
 * The CastMapBuilder analyzes LLVM bitcasts to identify casts between
 * structure types. This information is crucial for field-sensitive
 * pointer analysis to handle type punning correctly.
 * 
 * The analysis process:
 * 1. Collect all bitcasts in the module
 * 2. Compute the transitive closure of casts
 * 3. Extract structure type casts for pointer analysis
 */
class CastMapBuilder
{
private:
	const Module& module;
	CastMap& structCastMap;

	/**
	 * @brief Collects bitcast information from a single value
	 * 
	 * @param value The LLVM value to examine
	 * @param castMap The map to populate with cast information
	 * 
	 * Analyzes a value to see if it's a bitcast between pointer types.
	 * Excludes casts used only for memory intrinsics, as they don't
	 * represent true type conversions.
	 */
	void collectCast(const Value&, CastMap&);
	
	/**
	 * @brief Collects all bitcasts in the module
	 * 
	 * @return CastMap containing all pointer bitcasts
	 * 
	 * Examines global initializers and instructions in all functions
	 * to find bitcasts between pointer types.
	 */
	CastMap collectAllCasts();
	
	/**
	 * @brief Computes the transitive closure of casts
	 * 
	 * @param castMap The cast map to update with transitive casts
	 * 
	 * If type A can be cast to B and B can be cast to C, then
	 * this adds the information that A can be cast to C.
	 * This is important for understanding potential type confusion.
	 */
	void computeTransitiveClosure(CastMap&);
	
	/**
	 * @brief Extracts structure type casts from the general cast map
	 * 
	 * @param castMap The source cast map with all pointer casts
	 * 
	 * Filters the complete cast map to retain only casts between
	 * structure types, which are the focus of field-sensitive analysis.
	 */
	void extractStructs(CastMap&);
public:
	/**
	 * @brief Constructor for CastMapBuilder
	 * 
	 * @param m The LLVM module to analyze
	 * @param c The CastMap to populate with struct casts
	 */
	CastMapBuilder(const Module& m, CastMap& c): module(m), structCastMap(c) {}

	/**
	 * @brief Builds the complete structure cast map
	 * 
	 * This is the main method that orchestrates the cast analysis process.
	 */
	void buildCastMap();
};

/**
 * @brief Collects bitcast information from a single value
 * 
 * Examines a value to identify bitcasts between pointer types.
 * Filters out casts used only for memory intrinsics (memcpy, memmove),
 * as these don't represent true type conversions for analysis purposes.
 */
void CastMapBuilder::collectCast(const Value& value, CastMap& castMap)
{
	if (auto bc = dyn_cast<BitCastOperator>(&value))
	{
		auto srcType = bc->getSrcTy();
		auto dstType = bc->getDestTy();

		if (!srcType->isPointerTy() || !dstType->isPointerTy())
			return;

		// Sometimes, a value is casted only to satisfy the type signature of a particular API (e.g. llvm.memcpy). Exclude those cases
		if (bc->hasOneUse())
		{
			auto user = *bc->user_begin();
			if (isa<MemIntrinsic>(user))
				return;
		}

		castMap.insert(srcType, dstType);
	}
}

/**
 * @brief Collects all bitcasts in the module
 * 
 * Examines:
 * 1. Global variable initializers for constant expressions
 * 2. All instructions in all functions
 * 
 * This provides a comprehensive view of type casts in the program.
 */
CastMap CastMapBuilder::collectAllCasts()
{
	CastMap castMap;

	for (auto const& global: module.globals())
	{
		if (global.hasInitializer())
			collectCast(*global.getInitializer(), castMap);
	}

	for (auto const& f: module)
		for (auto const& bb: f)
			for (auto const& inst: bb)
				collectCast(inst, castMap);

	return castMap;
}

/**
 * @brief Computes the transitive closure of casts
 * 
 * Uses a fixpoint algorithm to compute all transitive casts:
 * If type A can be cast to B and B can be cast to C, then
 * A can be cast to C. This is important for understanding all
 * possible type relationships that may affect field sensitivity.
 */
void CastMapBuilder::computeTransitiveClosure(CastMap& castMap)
{
	// This is a really naive and inefficient way of computing transitive closure. However typically the size of the map won't be too large, hence such a method is very often acceptable. We'll try to optimize it once it becomes a problem
	bool changed;
	do
	{
		changed = false;
		for (auto& mapping: castMap)
		{
			// We need to make a copy here
			auto types = mapping.second;
			for (auto type: types)
			{
				auto itr = castMap.find(type);
				if (itr != castMap.end())
				{
					for (auto dstType: itr->second)
					{
						if (dstType != mapping.first)
							changed |= mapping.second.insert(dstType).second;
					}
				}
			}
		}
	} while (changed);
}

/**
 * @brief Extracts structure type casts from the general cast map
 * 
 * Filters the complete cast map to focus only on casts between
 * structure types, which are the primary concern for field-sensitive
 * pointer analysis. Ignores casts involving non-struct types.
 */
void CastMapBuilder::extractStructs(CastMap& castMap)
{
	for (auto const& mapping: castMap)
	{
		auto lhs = mapping.first->getPointerElementType();
		if (!lhs->isStructTy())
			continue;
		
		auto& rhsSet = structCastMap.getOrCreateRHS(lhs);
		for (auto dstType: mapping.second)
		{
			auto rhs = dstType->getPointerElementType();
			if (!rhs->isStructTy())
				continue;
			rhsSet.insert(rhs);
		}
	}
}

/**
 * @brief Builds the complete structure cast map
 * 
 * This is the main method that orchestrates the structure cast analysis:
 * 1. Collect all bitcasts in the module
 * 2. Compute the transitive closure of these casts
 * 3. Extract structure-to-structure casts for field-sensitive analysis
 */
void CastMapBuilder::buildCastMap()
{
	auto allCastMap = collectAllCasts();
	computeTransitiveClosure(allCastMap);
	extractStructs(allCastMap);
}

}

/**
 * @brief Analyzes structure type casts in an LLVM module
 * 
 * @param module The LLVM module to analyze
 * @return CastMap A mapping of structure types to other structure types they're cast to
 * 
 * This is the main entry point for structure cast analysis. It identifies
 * all places where structure types are cast to other structure types,
 * which is crucial for field-sensitive pointer analysis.
 * 
 * The resulting CastMap enables the pointer analysis to:
 * - Handle type punning correctly
 * - Maintain field sensitivity even with type casts
 * - Properly model memory access patterns with multiple type views
 */
CastMap StructCastAnalysis::runOnModule(const Module& module)
{
	CastMap structCastMap;

	CastMapBuilder(module, structCastMap).buildCastMap();

	return structCastMap;
}

}