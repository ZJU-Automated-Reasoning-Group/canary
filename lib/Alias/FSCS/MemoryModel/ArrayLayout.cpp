#include "Alias/FSCS/MemoryModel/Type/ArrayLayout.h"

namespace tpa
{

/**
 * Validates a list of array triples to ensure they form a valid array layout
 *
 * @param list The list of array triples to validate
 * @return True if the list is valid, false otherwise
 *
 * Ensures that:
 * 1. Each triple defines a valid range (start + size <= end)
 * 2. Each stride divides the range size evenly ((end - start) % size == 0)
 * 3. The list is sorted by start position (and by descending size for same start)
 * 4. No duplicate triples exist
 */
static bool validateTripleList(const ArrayLayout::ArrayTripleList& list)
{
	for (auto const& triple: list)
	{
		if (triple.start + triple.size > triple.end)
			return false;
		if ((triple.end - triple.start) % triple.size != 0)
			return false;
	}

	auto isSorted = std::is_sorted(
		list.begin(),
		list.end(),
		[] (auto const& lhs, auto const& rhs)
		{
			return
				(lhs.start < rhs.start) ||
				(lhs.start == rhs.start && lhs.size > rhs.size);
		}
	);

	if (!isSorted)
		return false;

	return std::unordered_set<ArrayTriple>(list.begin(), list.end()).size() == list.size();
}

/**
 * Creates an array layout from a list of array triples
 * 
 * @param list The list of array triples that define the layout
 * @return A pointer to the created ArrayLayout
 * 
 * Each triple in the list defines a region in memory with:
 * - start: beginning offset
 * - end: ending offset
 * - size: element size/stride
 * 
 * This method ensures only one instance of each unique layout exists
 * by using the layoutSet to check for existing instances.
 */
const ArrayLayout* ArrayLayout::getLayout(ArrayTripleList&& list)
{
	assert(validateTripleList(list));
	auto itr = layoutSet.insert(ArrayLayout(std::move(list))).first;
	return &(*itr);
}

/**
 * Creates an array layout from an initializer list of array triples
 * 
 * @param ilist An initializer list of array triples that define the layout
 * @return A pointer to the created ArrayLayout
 * 
 * Convenience method that converts an initializer list to an ArrayTripleList
 * and delegates to the primary getLayout method.
 */
const ArrayLayout* ArrayLayout::getLayout(std::initializer_list<ArrayTriple> ilist)
{
	ArrayTripleList list(ilist);
	return getLayout(std::move(list));
}

/**
 * Gets the byte array layout singleton instance
 * 
 * @return A pointer to the byte array layout
 * 
 * The byte array layout represents the most conservative array model,
 * treating memory as an array of bytes (size 1) with no upper bound.
 * This is used when the exact layout is unknown or for modeling void pointers.
 */
const ArrayLayout* ArrayLayout::getByteArrayLayout()
{
	return getLayout({{0, std::numeric_limits<size_t>::max(), 1}});
}

/**
 * Gets the default (empty) array layout singleton instance
 * 
 * @return A pointer to the default layout
 * 
 * The default layout represents non-array types (scalar or struct
 * that doesn't contain arrays). This is a performance optimization
 * to avoid creating multiple instances.
 */
const ArrayLayout* ArrayLayout::getDefaultLayout()
{
	return defaultLayout;
}

/**
 * Calculates the adjusted offset for array-sensitive pointer analysis
 * 
 * @param offset The original byte offset into the object
 * @return A pair with adjusted offset and whether an array was encountered
 * 
 * This method is key to field-sensitive pointer analysis:
 * - It finds if the offset hits any array region in the layout
 * - If so, it normalizes the offset to the appropriate array element
 * - The boolean result indicates if array summarization should be used
 */
std::pair<size_t, bool> ArrayLayout::offsetInto(size_t offset) const
{
	bool hitArray = false;
	for (auto const& triple: arrayLayout)
	{
		if (triple.start > offset)
			break;

		if (triple.start <= offset && offset < triple.end)
		{
			hitArray = true;
			offset = triple.start + (offset - triple.start) % triple.size;
		}
	}
	return std::make_pair(offset, hitArray);
}

}