#include "Alias/FSCS/FrontEnd/Type/ArrayLayoutAnalysis.h"
#include "Alias/FSCS/FrontEnd/Type/PointerLayoutAnalysis.h"
#include "Alias/FSCS/FrontEnd/Type/StructCastAnalysis.h"
#include "Alias/FSCS/FrontEnd/Type/TypeAnalysis.h"
#include "Alias/FSCS/FrontEnd/Type/TypeCollector.h"
#include "Alias/FSCS/MemoryModel/Type/TypeLayout.h"

#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

namespace
{

/**
 * @brief Helper class to build a TypeMap for pointer analysis
 *
 * The TypeMapBuilder analyzes LLVM types to create appropriate TypeLayout
 * objects for pointer analysis. It integrates information from various
 * specialized analyses:
 * - TypeCollector: Gathers all relevant types from the module
 * - StructCastAnalysis: Identifies type casts for field-sensitive handling
 * - ArrayLayoutAnalysis: Provides array structure information
 * - PointerLayoutAnalysis: Identifies pointer fields within types
 *
 * The resulting TypeMap enables field-sensitive pointer analysis by
 * providing size and layout information for all types.
 */
class TypeMapBuilder
{
private:
	const Module& module;
	TypeMap& typeMap;

	/**
	 * @brief Gets the size in bytes of a type
	 *
	 * @param type The LLVM type
	 * @param dataLayout The target data layout
	 * @return Size of the type in bytes
	 *
	 * Handles special cases like function types and arrays,
	 * using the target data layout for accurate sizing.
	 */
	size_t getTypeSize(Type*, const DataLayout&);
	
	/**
	 * @brief Inserts a type with its layout into the type map
	 *
	 * @param type The LLVM type
	 * @param size Size of the type in bytes
	 * @param arrayLayout Array layout information
	 * @param ptrLayout Pointer layout information
	 */
	void insertTypeMap(Type*, size_t, const ArrayLayout*, const PointerLayout*);
	
	/**
	 * @brief Inserts an opaque type into the type map
	 *
	 * @param type The LLVM opaque type
	 *
	 * Opaque types (whose internal structure is unknown) are
	 * conservatively modeled as byte arrays.
	 */
	void insertOpaqueType(Type*);
public:
	/**
	 * @brief Constructor for TypeMapBuilder
	 *
	 * @param m The LLVM module being analyzed
	 * @param t The TypeMap to populate
	 */
	TypeMapBuilder(const Module& m, TypeMap& t): module(m), typeMap(t) {}

	/**
	 * @brief Builds a complete type map for the module
	 *
	 * This is the main method that orchestrates the type analysis.
	 * It collects types, performs specialized analyses, and builds
	 * a comprehensive type map for pointer analysis.
	 */
	void buildTypeMap();
};

/**
 * @brief Inserts an opaque type into the type map
 *
 * Opaque types are modeled as byte arrays, treating them as
 * unstructured memory since their internal structure is unknown.
 */
void TypeMapBuilder::insertOpaqueType(Type* type)
{
	typeMap.insert(type, TypeLayout::getByteArrayTypeLayout());
}

/**
 * @brief Inserts a type with its layout into the type map
 *
 * Creates a TypeLayout object with the specified properties
 * and adds it to the type map for later use in pointer analysis.
 */
void TypeMapBuilder::insertTypeMap(Type* type, size_t size, const ArrayLayout* arrayLayout, const PointerLayout* ptrLayout)
{
	auto typeLayout = TypeLayout::getTypeLayout(size, arrayLayout, ptrLayout);
	typeMap.insert(type, typeLayout);
}

/**
 * @brief Gets the size in bytes of a type
 *
 * Handles special cases:
 * - Function types: Returns pointer size
 * - Array types: Looks at element type
 * - Unsized types: Returns pointer size
 * - Others: Uses data layout for accurate size information
 */
size_t TypeMapBuilder::getTypeSize(Type* type, const DataLayout& dataLayout)
{
	if (isa<FunctionType>(type))
		return dataLayout.getPointerSize();
	else
	{
		while (auto arrayType = dyn_cast<ArrayType>(type))
			type = arrayType->getElementType();
		
		if (!type->isSized())
			return dataLayout.getPointerSize();
		
		return dataLayout.getTypeAllocSize(type);
	}
}

/**
 * @brief Builds a complete type map for the module
 *
 * Process:
 * 1. Collect all relevant types from the module
 * 2. Analyze struct casts for field-sensitive handling
 * 3. Analyze array layouts to identify element patterns
 * 4. Analyze pointer layouts to identify pointer fields
 * 5. Create type layouts for all types, handling special cases
 *
 * The resulting type map provides comprehensive layout information
 * for field-sensitive pointer analysis.
 */
void TypeMapBuilder::buildTypeMap()
{
	auto typeSet = TypeCollector().runOnModule(module);
	auto structCastMap = StructCastAnalysis().runOnModule(module);
	auto arrayLayoutMap = ArrayLayoutAnalysis().runOnTypes(typeSet);
	auto ptrLayoutMap = PointerLayoutAnalysis(structCastMap).runOnTypes(typeSet);

	for (auto type: typeSet)
	{
		if (auto stType = dyn_cast<StructType>(type))
		{
			if (stType->isOpaque())
			{
				insertOpaqueType(type);
				continue;
			}
		}
		
		if (!type->isSized())
		{
			insertOpaqueType(type);
			continue;
		}

		auto typeSize = getTypeSize(type, typeSet.getDataLayout());

		auto ptrLayout = ptrLayoutMap.lookup(type);
		assert(ptrLayout != nullptr);

		auto arrayLayout = arrayLayoutMap.lookup(type);
		assert(arrayLayout != nullptr);

		insertTypeMap(type, typeSize, arrayLayout, ptrLayout);
	}
}

}

/**
 * @brief Performs complete type analysis on an LLVM module
 *
 * @param module The LLVM module to analyze
 * @return TypeMap A mapping from LLVM types to TypeLayout objects
 *
 * This is the main entry point for type analysis. It creates a
 * TypeMapBuilder to analyze the module and build a comprehensive
 * type map for field-sensitive pointer analysis.
 *
 * The resulting TypeMap enables the pointer analysis to:
 * - Track field-sensitive points-to information
 * - Handle complex data structures accurately
 * - Model memory layout correctly for different targets
 */
TypeMap TypeAnalysis::runOnModule(const Module& module)
{
	TypeMap typeMap;

	TypeMapBuilder(module, typeMap).buildTypeMap();

	return typeMap;
}

}
