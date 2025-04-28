#pragma once

#include "Alias/FSCS/MemoryModel/Pointer.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace llvm
{
	class ConstantPointerNull;
	class Value;
	class UndefValue;
}

namespace tpa
{

// A class that is responsible for allocating pointers for each context and maintaining the mapping between Pointer and llvm::Value
class PointerManager
{
private:
	std::unordered_set<Pointer> ptrSet;

	// Convention: uPtr's value is a i8* UndefValue; nPtr's value is a i8* ConstantPointerNull; both of them has GlobalContext
	const Pointer* uPtr;
	const Pointer* nPtr;
	
	// Whether to preserve contexts for global values in context-sensitive analysis
	bool preserveGlobalValueContexts;

	// Group together Pointers with the same llvm::Value
	using PointerVector = std::vector<const Pointer*>;
	std::unordered_map<const llvm::Value*, PointerVector> valuePtrMap;

	const Pointer* buildPointer(const context::Context* ctx, const llvm::Value* val);
public:
	PointerManager();

	const Pointer* setUniversalPointer(const llvm::UndefValue*);
	const Pointer* getUniversalPointer() const;
	const Pointer* setNullPointer(const llvm::ConstantPointerNull*);
	const Pointer* getNullPointer() const;

	// Set whether to preserve contexts for global values
	void setPreserveGlobalValueContexts(bool preserve) { preserveGlobalValueContexts = preserve; }
	bool getPreserveGlobalValueContexts() const { return preserveGlobalValueContexts; }

	// Return a Pointer corresponds to (ctx, val). If not exist, create one
	const Pointer* getOrCreatePointer(const context::Context* ctx, const llvm::Value* val);
	// Return a Pointer corresponds to (ctx, val). If not exist, return NULL
	const Pointer* getPointer(const context::Context* ctx, const llvm::Value* val) const;
	// Return a vector of Pointers, whose elements corresponds to the same llvm::Value. Return empty vector if no such Pointer is found
	PointerVector getPointersWithValue(const llvm::Value* val) const;
	
	// Return all pointers managed by this PointerManager
	std::vector<const Pointer*> getAllPointers() const;
};

}