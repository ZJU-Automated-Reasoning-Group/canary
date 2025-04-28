#include "Alias/FSCS/MemoryModel/MemoryManager.h"
#include "Alias/FSCS/MemoryModel/Type/TypeLayout.h"

using namespace context;

namespace tpa
{

/**
 * Helper function to determine if a memory object should start with summary representation.
 * Summary objects represent multiple concrete memory locations as a single abstraction
 * to improve analysis efficiency.
 */
static bool startWithSummary(const TypeLayout* type)
{
	bool ret;
	std::tie(std::ignore, ret) = type->offsetInto(0);
	return ret;
}

/**
 * MemoryManager constructor
 * 
 * @param pSize The size of pointers in the target architecture
 * 
 * The MemoryManager is responsible for creating and managing abstract memory objects
 * used in the pointer analysis. It handles different types of memory allocations
 * (global, stack, heap) and provides operations for manipulating memory objects.
 */
MemoryManager::MemoryManager(size_t pSize): ptrSize(pSize), argvObj(nullptr), envpObj(nullptr)
{
}

/**
 * Gets or creates a memory object for a specific memory block and offset
 * 
 * @param memBlock The memory block (allocation site)
 * @param offset Offset within the memory block
 * @param summary Whether this is a summary object (representing multiple locations)
 * @return A pointer to the MemoryObject (owned by the MemoryManager)
 * 
 * This method ensures a single instance of each unique memory object exists,
 * supporting the analysis with a canonical representation.
 */
const MemoryObject* MemoryManager::getMemoryObject(const MemoryBlock* memBlock, size_t offset, bool summary) const
{
	assert(memBlock != nullptr);

	auto obj = MemoryObject(memBlock, offset, summary);
	auto itr = objSet.insert(obj).first;
	assert(itr->isSummaryObject() == summary);
	return &*itr;
}

/**
 * Allocates a memory block for a given allocation site and type
 * 
 * @param allocSite The allocation site identifier
 * @param type The type layout of the allocated memory
 * @return A pointer to the allocated MemoryBlock
 * 
 * Memory blocks represent allocation sites in the program and serve as
 * the base for all memory objects derived from them.
 */
const MemoryBlock* MemoryManager::allocateMemoryBlock(AllocSite allocSite, const TypeLayout* type)
{
	auto itr = allocMap.find(allocSite);
	if (itr == allocMap.end())
		itr = allocMap.insert(itr, std::make_pair(allocSite, MemoryBlock(allocSite, type)));
	assert(type == itr->second.getTypeLayout());
	return &itr->second;
}

/**
 * Allocates a memory object for a global variable
 * 
 * @param value The LLVM global variable
 * @param type The type layout of the global variable
 * @return A pointer to the allocated MemoryObject
 * 
 * Global variables are modeled as memory objects with a specific allocation site
 * and type. They exist throughout program execution.
 */
const MemoryObject* MemoryManager::allocateGlobalMemory(const llvm::GlobalVariable* value, const TypeLayout* type)
{
	assert(value != nullptr && type != nullptr);

	auto memBlock = allocateMemoryBlock(AllocSite::getGlobalAllocSite(value), type);
	return getMemoryObject(memBlock, 0, startWithSummary(type));
}

/**
 * Allocates a memory object for a function
 * 
 * @param f The LLVM function
 * @return A pointer to the allocated MemoryObject
 * 
 * Functions are also represented as memory objects in the analysis,
 * allowing them to be targets of function pointers.
 */
const MemoryObject* MemoryManager::allocateMemoryForFunction(const llvm::Function* f)
{
	auto memBlock = allocateMemoryBlock(AllocSite::getFunctionAllocSite(f), TypeLayout::getPointerTypeLayoutWithSize(0));
	return getMemoryObject(memBlock, 0, false);
}

/**
 * Allocates a memory object for a stack allocation
 * 
 * @param ctx The context in which the allocation occurs
 * @param ptr The LLVM value representing the allocation
 * @param type The type layout of the allocated memory
 * @return A pointer to the allocated MemoryObject
 * 
 * Stack allocations are context-sensitive, allowing the analysis to
 * distinguish between the same stack variable in different calling contexts.
 */
const MemoryObject* MemoryManager::allocateStackMemory(const Context* ctx, const llvm::Value* ptr, const TypeLayout* type)
{
	auto memBlock = allocateMemoryBlock(AllocSite::getStackAllocSite(ctx, ptr), type);
	return getMemoryObject(memBlock, 0, startWithSummary(type));
}

/**
 * Allocates a memory object for a heap allocation
 * 
 * @param ctx The context in which the allocation occurs
 * @param ptr The LLVM value representing the allocation
 * @param type The type layout of the allocated memory
 * @return A pointer to the allocated MemoryObject
 * 
 * Heap allocations are modeled as summary objects by default, since heap
 * structures often contain arrays or repeated elements that are efficiently
 * modeled with summarization.
 */
const MemoryObject* MemoryManager::allocateHeapMemory(const Context* ctx, const llvm::Value* ptr, const TypeLayout* type)
{
	auto memBlock = allocateMemoryBlock(AllocSite::getHeapAllocSite(ctx, ptr), type);
	return getMemoryObject(memBlock, 0, true);
}

/**
 * Allocates a memory object for the argv array in main()
 * 
 * @param ptr The LLVM value representing argv
 * @return A pointer to the allocated MemoryObject
 * 
 * Special handling for the argv parameter to main(), which is
 * modeled as a byte array with summary representation.
 */
const MemoryObject* MemoryManager::allocateArgv(const llvm::Value* ptr)
{
	auto memBlock = allocateMemoryBlock(AllocSite::getStackAllocSite(Context::getGlobalContext(), ptr), TypeLayout::getByteArrayTypeLayout());
	argvObj = getMemoryObject(memBlock, 0, true);
	return argvObj;
}

/**
 * Allocates a memory object for the envp array in main()
 * 
 * @param ptr The LLVM value representing envp
 * @return A pointer to the allocated MemoryObject
 * 
 * Special handling for the envp parameter to main(), which is
 * modeled as a byte array with summary representation.
 */
const MemoryObject* MemoryManager::allocateEnvp(const llvm::Value* ptr)
{
	auto memBlock = allocateMemoryBlock(AllocSite::getStackAllocSite(Context::getGlobalContext(), ptr), TypeLayout::getByteArrayTypeLayout());
	envpObj = getMemoryObject(memBlock, 0, true);
	return envpObj;
}

/**
 * Creates a memory object at a specific offset from an existing memory object
 * 
 * @param obj The base memory object
 * @param offset The offset to add
 * @return A pointer to the resulting MemoryObject
 * 
 * This method is used for field access, array indexing, and pointer arithmetic.
 * It handles field sensitivity by creating precise memory objects for struct fields.
 */
const MemoryObject* MemoryManager::offsetMemory(const MemoryObject* obj, size_t offset) const
{
	assert(obj != nullptr);

	if (offset == 0)
		return obj;
	else
		return offsetMemory(obj->getMemoryBlock(), obj->getOffset() + offset);
}

/**
 * Creates a memory object at a specific offset from a memory block
 * 
 * @param block The base memory block
 * @param offset The absolute offset within the block
 * @return A pointer to the resulting MemoryObject
 * 
 * This method handles the complexities of field-sensitive modeling,
 * including bounds checking and determining when to use summary objects.
 */
const MemoryObject* MemoryManager::offsetMemory(const MemoryBlock* block, size_t offset) const
{
	assert(block != nullptr);

	if (block == &uBlock || block == &nBlock)
		return &uObj;

	auto type = block->getTypeLayout();

	bool summary;
	size_t adjustedOffset;
	std::tie(adjustedOffset, summary) = type->offsetInto(offset);
	// Heap objects are always summary
	summary = summary || block->isHeapBlock();

	if (adjustedOffset >= type->getSize())
		return &uObj;

	return getMemoryObject(block, adjustedOffset, summary);
}

/**
 * Gets all memory objects that can be reached as pointers from a given memory object
 * 
 * @param obj The source memory object
 * @param includeSelf Whether to include the source object in the result
 * @return A vector of memory objects that can be reached
 * 
 * This method is used for points-to analysis to find all memory objects
 * that could contain pointers, starting from a given object.
 */
std::vector<const MemoryObject*> MemoryManager::getReachablePointerObjects(const MemoryObject* obj, bool includeSelf) const
{
	auto ret = std::vector<const MemoryObject*>();
	if (includeSelf)
		ret.push_back(obj);

	if (!obj->isSpecialObject())
	{
		auto memBlock = obj->getMemoryBlock();
		auto ptrLayout = memBlock->getTypeLayout()->getPointerLayout();
		auto itr = ptrLayout->lower_bound(obj->getOffset());
		if (itr != ptrLayout->end() && *itr == obj->getOffset())
			++itr;
		std::transform(
			itr,
			ptrLayout->end(),
			std::back_inserter(ret),
			[this, memBlock] (size_t offset)
			{
				return offsetMemory(memBlock, offset);
			}
		);
	}

	return ret;
}

/**
 * Gets all memory objects that are part of the same allocation as a given memory object
 * 
 * @param obj The source memory object
 * @return A vector of memory objects in the same allocation
 * 
 * This method retrieves all memory objects that belong to the same memory block,
 * which is useful for analyzing compound objects with multiple fields.
 */
std::vector<const MemoryObject*> MemoryManager::getReachableMemoryObjects(const MemoryObject* obj) const
{
	auto ret = std::vector<const MemoryObject*>();

	if (obj->isSpecialObject())
	{
		ret.push_back(obj);
	}
	else
	{
		auto itr = objSet.find(*obj);
		assert(itr != objSet.end());

		auto block = obj->getMemoryBlock();
		while (itr != objSet.end() && itr->getMemoryBlock() == block)
		{
			ret.push_back(&*itr);
			++itr;
		}
	}

	return ret;
}

}