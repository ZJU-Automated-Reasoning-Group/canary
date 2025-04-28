#include "Alias/FSCS/MemoryModel/Type/TypeLayout.h"

namespace tpa
{

/**
 * Creates a type layout using initializer lists for array and pointer layouts
 * 
 * @param s The size of the type in bytes
 * @param a Initializer list of array triples for array-sensitive analysis
 * @param p Initializer list of offsets for pointer-sensitive analysis
 * @return A pointer to the created TypeLayout
 * 
 * Convenience method that creates ArrayLayout and PointerLayout instances
 * from the initializer lists and passes them to the main getTypeLayout method.
 */
const TypeLayout* TypeLayout::getTypeLayout(size_t s, std::initializer_list<ArrayTriple> a, std::initializer_list<size_t> p)
{
	return getTypeLayout(s, ArrayLayout::getLayout(std::move(a)), PointerLayout::getLayout(std::move(p)));
}

/**
 * Creates a type layout with specified array and pointer layouts
 * 
 * @param size The size of the type in bytes
 * @param a Pointer to the ArrayLayout to use
 * @param p Pointer to the PointerLayout to use
 * @return A pointer to the created TypeLayout
 * 
 * This method ensures only one instance of each unique layout exists
 * by using the typeSet to check for existing instances.
 */
const TypeLayout* TypeLayout::getTypeLayout(size_t size, const ArrayLayout* a, const PointerLayout* p)
{
	assert(a != nullptr && p != nullptr);

	auto itr = typeSet.insert(TypeLayout(size, a, p)).first;
	return &(*itr);
}

/**
 * Creates a type layout for an array type
 * 
 * @param elemLayout The type layout of the array element
 * @param elemCount The number of elements in the array
 * @return A pointer to the created TypeLayout for the array
 * 
 * This creates a composite layout for arrays that:
 * 1. Sets the overall size based on element size × count
 * 2. Creates a new array layout that treats the whole array as an array of elements
 * 3. Preserves the array properties of the element type
 * 4. Preserves the pointer layout of the element type
 */
const TypeLayout* TypeLayout::getArrayTypeLayout(const TypeLayout* elemLayout, size_t elemCount)
{
	assert(elemLayout != nullptr);
	auto elemArrayLayout = elemLayout->getArrayLayout();
	auto elemSize = elemLayout->getSize();
	auto newSize = elemSize * elemCount;

	auto arrayTripleList = ArrayLayout::ArrayTripleList();
	arrayTripleList.reserve(elemArrayLayout->size() + 1);
	arrayTripleList.push_back({ 0, newSize, elemSize });
	for (auto const& triple: *elemArrayLayout)
		arrayTripleList.push_back(triple);
	auto newArrayLayout = ArrayLayout::getLayout(std::move(arrayTripleList));

	return getTypeLayout(newSize, newArrayLayout, elemLayout->getPointerLayout());
}

/**
 * Creates a type layout for a pointer type
 * 
 * @param size The size of the pointer in bytes
 * @return A pointer to the created TypeLayout for the pointer
 * 
 * This creates a layout for pointer types that:
 * 1. Uses the default array layout (not an array)
 * 2. Uses the single pointer layout (pointer at offset 0)
 */
const TypeLayout* TypeLayout::getPointerTypeLayoutWithSize(size_t size)
{
	return getTypeLayout(size, ArrayLayout::getDefaultLayout(), PointerLayout::getSinglePointerLayout());
}

/**
 * Creates a type layout for a non-pointer type
 * 
 * @param size The size of the type in bytes
 * @return A pointer to the created TypeLayout for the non-pointer type
 * 
 * This creates a layout for primitive, non-pointer types that:
 * 1. Uses the default array layout (not an array)
 * 2. Uses the empty pointer layout (contains no pointers)
 */
const TypeLayout* TypeLayout::getNonPointerTypeLayoutWithSize(size_t size)
{
	return getTypeLayout(size, ArrayLayout::getDefaultLayout(), PointerLayout::getEmptyLayout());
}

/**
 * Creates a type layout for a byte array
 * 
 * @return A pointer to the created TypeLayout for the byte array
 * 
 * This creates a conservative layout that:
 * 1. Has a size of 1 byte
 * 2. Uses the byte array layout (array of bytes)
 * 3. Uses the single pointer layout (can contain pointers)
 * 
 * This is used for void pointers, opaque types, and other cases
 * where the exact structure is unknown.
 */
const TypeLayout* TypeLayout::getByteArrayTypeLayout()
{
	return getTypeLayout(1, ArrayLayout::getByteArrayLayout(), PointerLayout::getSinglePointerLayout());
}

/**
 * Calculates the adjusted offset for array-sensitive pointer analysis
 * 
 * @param size The original byte offset into the object
 * @return A pair with adjusted offset and whether an array was encountered
 * 
 * Delegates to the ArrayLayout to perform the offset adjustment,
 * which is key to field-sensitive pointer analysis.
 */
std::pair<size_t, bool> TypeLayout::offsetInto(size_t size) const
{
	return arrayLayout->offsetInto(size);
}

}