#include "Alias/FSCS/MemoryModel/Type/PointerLayout.h"

namespace tpa
{

/**
 * Gets the empty pointer layout singleton instance
 * 
 * @return A pointer to the empty pointer layout
 * 
 * The empty layout is used to represent types that do not contain any pointers.
 * This is a performance optimization to avoid creating multiple instances.
 */
const PointerLayout* PointerLayout::getEmptyLayout()
{
	return emptyLayout;
}

/**
 * Gets the single pointer layout singleton instance
 * 
 * @return A pointer to the single pointer layout
 * 
 * The single pointer layout represents a type that contains a pointer at offset 0.
 * This is commonly used for pointer types in the analysis.
 */
const PointerLayout* PointerLayout::getSinglePointerLayout()
{
	return singlePointerLayout;
}

/**
 * Creates a pointer layout from a set of valid offsets
 * 
 * @param set A set of byte offsets where pointers can be found
 * @return A pointer to the created PointerLayout
 * 
 * This method ensures only one instance of each unique layout exists
 * by using the layoutSet to check for existing instances.
 */
const PointerLayout* PointerLayout::getLayout(SetType&& set)
{
	auto itr = layoutSet.insert(PointerLayout(std::move(set))).first;
	return &(*itr);
}

/**
 * Creates a pointer layout from a list of valid offsets
 * 
 * @param ilist An initializer list of byte offsets where pointers can be found
 * @return A pointer to the created PointerLayout
 * 
 * Convenience method that converts an initializer list to a SetType
 * and delegates to the primary getLayout method.
 */
const PointerLayout* PointerLayout::getLayout(std::initializer_list<size_t> ilist)
{
	SetType set(ilist);
	return getLayout(std::move(set));
}

/**
 * Merges two pointer layouts
 * 
 * @param lhs First pointer layout
 * @param rhs Second pointer layout
 * @return A pointer to the merged layout
 * 
 * Creates a new pointer layout containing all offsets from both input layouts.
 * This is used during pointer analysis to handle type casts and unions.
 * Handles special cases like identical inputs and empty layouts efficiently.
 */
const PointerLayout* PointerLayout::merge(const PointerLayout* lhs, const PointerLayout* rhs)
{
	assert(lhs != nullptr && rhs != nullptr);

	if (lhs == rhs)
		return lhs;
	if (lhs->empty())
		return rhs;
	if (rhs->empty())
		return lhs;

	SetType newSet(lhs->validOffsets);
	newSet.merge(rhs->validOffsets);
	return getLayout(std::move(newSet));
}

}