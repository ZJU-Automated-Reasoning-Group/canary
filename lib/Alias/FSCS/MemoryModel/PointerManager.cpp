#include "Alias/FSCS/Context/Context.h"
#include "Alias/FSCS/Context/KLimitContext.h"
#include "Alias/FSCS/MemoryModel/PointerManager.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>

using namespace context;

namespace tpa
{

/**
 * Canonicalizes an LLVM Value for pointer analysis
 * 
 * @param value The LLVM value to canonicalize
 * @return The canonicalized form of the value
 * 
 * This helper function performs various transformations to normalize values:
 * 1. Strips pointer casts to get to the original value
 * 2. Simplifies PHI nodes with identical operands
 * 3. Replaces int-to-ptr instructions with undef values
 * 
 * Canonicalization helps ensure that equivalent pointer values
 * are treated as equivalent in the analysis.
 */
const llvm::Value* canonicalizeValue(const llvm::Value* value)
{
	assert(value != nullptr);
	value = value->stripPointerCasts();
	if (auto phiNode = llvm::dyn_cast<llvm::PHINode>(value))
	{
		const llvm::Value* rhs = nullptr;
		for (auto& op: phiNode->operands())
		{
			auto val = op.get()->stripPointerCasts();
			if (rhs == nullptr)
				rhs = val;
			else if (val != rhs)
			{
				rhs = nullptr;
				break;
			}
		}

		if (rhs != nullptr)
			value = rhs;
	}
	else if (llvm::isa<llvm::IntToPtrInst>(value))
		value = llvm::UndefValue::get(value->getType());

	return value;
}

/**
 * Constructs a PointerManager instance
 * 
 * Initializes the universal and null pointer fields to nullptr.
 * These special pointers need to be set explicitly using setUniversalPointer
 * and setNullPointer before the manager can be fully used.
 */
PointerManager::PointerManager(): uPtr(nullptr), nPtr(nullptr), preserveGlobalValueContexts(false) {}

/**
 * Builds or retrieves a Pointer for a context-value pair
 * 
 * @param ctx The context of the pointer
 * @param val The LLVM value representing the pointer
 * @return A pointer to the created or existing Pointer object
 * 
 * This internal method ensures a single instance exists for each
 * unique context-value pair. It updates the valuePtrMap for reverse lookups.
 */
const Pointer* PointerManager::buildPointer(const context::Context* ctx, const llvm::Value* val)
{
	auto ptr = Pointer(ctx, val);
	auto itr = ptrSet.find(ptr);
	if (itr != ptrSet.end())
		return &*itr;

	itr = ptrSet.insert(itr, ptr);
	auto ret = &*itr;
	valuePtrMap[val].push_back(ret);
	return ret;
}

/**
 * Sets the universal pointer value
 * 
 * @param v The LLVM undefined value to represent the universal pointer
 * @return A pointer to the universal pointer object
 * 
 * The universal pointer is used to represent unknown pointer values
 * or pointers that could point to any memory location. This is essential
 * for soundly handling undefined behavior and complex pointer operations.
 */
const Pointer* PointerManager::setUniversalPointer(const llvm::UndefValue* v)
{
	assert(uPtr == nullptr);
	assert(v->getType() == llvm::Type::getInt8PtrTy(v->getContext()));
	uPtr = buildPointer(Context::getGlobalContext(), v);
	return uPtr;
}

/**
 * Gets the universal pointer instance
 * 
 * @return A pointer to the universal pointer object
 * 
 * This pointer represents an unknown pointer value that could
 * point to any memory location. Must be set before being accessed.
 */
const Pointer* PointerManager::getUniversalPointer() const
{
	assert(uPtr != nullptr);
	return uPtr;
}

/**
 * Sets the null pointer value
 * 
 * @param v The LLVM null pointer constant
 * @return A pointer to the null pointer object
 * 
 * The null pointer represents the NULL value in C/C++.
 * It must be set before the pointer manager can fully function.
 */
const Pointer* PointerManager::setNullPointer(const llvm::ConstantPointerNull* v)
{
	assert(nPtr == nullptr);
	assert(v->getType() == llvm::Type::getInt8PtrTy(v->getContext()));
	nPtr = buildPointer(Context::getGlobalContext(), v);
	return nPtr;
}

/**
 * Gets the null pointer instance
 * 
 * @return A pointer to the null pointer object
 * 
 * This pointer represents the NULL value in C/C++.
 * Must be set before being accessed.
 */
const Pointer* PointerManager::getNullPointer() const
{
	assert(nPtr != nullptr);
	return nPtr;
}

/**
 * Looks up a pointer for a context-value pair
 * 
 * @param ctx The context of the pointer
 * @param val The LLVM value representing the pointer
 * @return A pointer to the Pointer object or nullptr if not found
 * 
 * This method handles special cases like:
 * 1. Canonicalizing the value to ensure consistent lookup
 * 2. Using the null or universal pointer for null or undefined values
 * 3. Using the global context for global values when preserveGlobalValueContexts is false
 */
const Pointer* PointerManager::getPointer(const Context* ctx, const llvm::Value* val) const
{
	assert(ctx != nullptr && val != nullptr);

	val = canonicalizeValue(val);
	
	if (llvm::isa<llvm::ConstantPointerNull>(val))
		return nPtr;
	else if (llvm::isa<llvm::UndefValue>(val))
		return uPtr;
	else if (llvm::isa<llvm::GlobalValue>(val)) {
		// Only reset context for globals if context preservation is disabled
		// or if k-limit is 0 (context-insensitive analysis)
		if (!preserveGlobalValueContexts || context::KLimitContext::getLimit() == 0) {
			ctx = Context::getGlobalContext();
		}
	}

	auto itr = ptrSet.find(Pointer(ctx, val));
	if (itr == ptrSet.end())
		return nullptr;
	else
		return &*itr;
}

/**
 * Gets or creates a pointer for a context-value pair
 * 
 * @param ctx The context of the pointer
 * @param val The LLVM value representing the pointer
 * @return A pointer to the created or existing Pointer object
 * 
 * Similar to getPointer, but ensures a Pointer is created if it
 * doesn't already exist. Used during points-to analysis to create
 * pointers on demand.
 */
const Pointer* PointerManager::getOrCreatePointer(const Context* ctx, const llvm::Value* val)
{
	assert(ctx != nullptr && val != nullptr);

	val = canonicalizeValue(val);

	if (llvm::isa<llvm::ConstantPointerNull>(val))
		return nPtr;
	else if (llvm::isa<llvm::UndefValue>(val))
		return uPtr;
	else if (llvm::isa<llvm::GlobalValue>(val)) {
		// Only reset context for globals if context preservation is disabled
		// or if k-limit is 0 (context-insensitive analysis)
		if (!preserveGlobalValueContexts || context::KLimitContext::getLimit() == 0) {
			ctx = Context::getGlobalContext();
		}
	}

	return buildPointer(ctx, val);
}

/**
 * Finds all Pointer objects associated with a specific LLVM value
 * 
 * @param val The LLVM value to look up
 * @return A vector of pointers to Pointer objects
 * 
 * This retrieves pointers across all contexts for a single value.
 * Useful for call graph construction and cross-context analysis.
 */
PointerManager::PointerVector PointerManager::getPointersWithValue(const llvm::Value* val) const
{
	PointerVector vec;

	val = canonicalizeValue(val);

	if (llvm::isa<llvm::ConstantPointerNull>(val))
		vec.push_back(nPtr);
	else if (llvm::isa<llvm::UndefValue>(val))
		vec.push_back(uPtr);
	else
	{
		auto itr = valuePtrMap.find(val);
		if (itr != valuePtrMap.end())
			vec = itr->second;
	}

	return vec;
}

/**
 * Retrieves all Pointer objects managed by this instance
 * 
 * @return A vector of all pointers in the manager
 * 
 * Used for iteration over all pointers in the analysis,
 * such as during result reporting or cleanup.
 */
std::vector<const Pointer*> PointerManager::getAllPointers() const
{
    std::vector<const Pointer*> result;
    result.reserve(ptrSet.size());
    
    for (const auto& ptr : ptrSet) {
        result.push_back(&ptr);
    }
    
    return result;
}

}