#include "Alias/FSCS/Analysis/PointerAnalysisQueries.h"
#include <algorithm>
#include <unordered_set>

namespace tpa
{

// Points-to queries implementation

PtsSet PointerAnalysisQueries::getPointsToSet(const context::Context* ctx, const llvm::Value* val) const
{
    if (!ctx || !val) {
        return PtsSet::getEmptySet();
    }
    
    const Pointer* ptr = ptrManager.getPointer(ctx, val->stripPointerCasts());
    if (!ptr) {
        return PtsSet::getEmptySet();
    }
    
    return getPointsToSet(ptr);
}

PtsSet PointerAnalysisQueries::getPointsToSet(const llvm::Value* val) const
{
    if (!val) {
        return PtsSet::getEmptySet();
    }
    
    auto ptrs = ptrManager.getPointersWithValue(val->stripPointerCasts());
    if (ptrs.empty()) {
        return PtsSet::getEmptySet();
    }
    
    std::vector<PtsSet> pSets;
    pSets.reserve(ptrs.size());
    
    for (auto ptr : ptrs) {
        pSets.emplace_back(getPointsToSet(ptr));
    }
    
    return PtsSet::mergeAll(pSets);
}

PtsSet PointerAnalysisQueries::getPointsToSet(const Pointer* ptr) const
{
    if (!ptr) {
        return PtsSet::getEmptySet();
    }
    
    return getPtsSetForPointer(ptr);
}

bool PointerAnalysisQueries::mayPointTo(const Pointer* ptr, const MemoryObject* obj) const
{
    if (!ptr || !obj) {
        return false;
    }
    
    PtsSet pts = getPointsToSet(ptr);
    return pts.has(obj);
}

bool PointerAnalysisQueries::mayPointTo(const context::Context* ctx, const llvm::Value* val, 
                                       const MemoryObject* obj) const
{
    if (!ctx || !val || !obj) {
        return false;
    }
    
    const Pointer* ptr = ptrManager.getPointer(ctx, val->stripPointerCasts());
    if (!ptr) {
        return false;
    }
    
    return mayPointTo(ptr, obj);
}

// Alias queries implementation

bool PointerAnalysisQueries::mayAlias(const Pointer* ptr1, const Pointer* ptr2) const
{
    if (!ptr1 || !ptr2) {
        return false;
    }
    
    // Get points-to sets
    PtsSet pts1 = getPointsToSet(ptr1);
    PtsSet pts2 = getPointsToSet(ptr2);
    
    // Check if points-to sets have non-empty intersection
    // Note: if either points to universal object, they alias
    if (pts1.has(MemoryManager::getUniversalObject()) || 
        pts2.has(MemoryManager::getUniversalObject())) {
        return true;
    }
    
    // Check for common objects
    auto common = PtsSet::intersects(pts1, pts2);
    return !common.empty();
}

bool PointerAnalysisQueries::mayAlias(const context::Context* ctx1, const llvm::Value* val1, 
                                     const context::Context* ctx2, const llvm::Value* val2) const
{
    if (!ctx1 || !val1 || !ctx2 || !val2) {
        return false;
    }
    
    const Pointer* ptr1 = ptrManager.getPointer(ctx1, val1->stripPointerCasts());
    const Pointer* ptr2 = ptrManager.getPointer(ctx2, val2->stripPointerCasts());
    
    if (!ptr1 || !ptr2) {
        return false;
    }
    
    return mayAlias(ptr1, ptr2);
}

bool PointerAnalysisQueries::mayAlias(const llvm::Value* val1, const llvm::Value* val2) const
{
    if (!val1 || !val2) {
        return false;
    }
    
    // Get all contexts for these values
    auto ptrs1 = ptrManager.getPointersWithValue(val1->stripPointerCasts());
    auto ptrs2 = ptrManager.getPointersWithValue(val2->stripPointerCasts());
    
    // Check if any pair of pointers may alias
    for (auto ptr1 : ptrs1) {
        for (auto ptr2 : ptrs2) {
            if (mayAlias(ptr1, ptr2)) {
                return true;
            }
        }
    }
    
    return false;
}

// Pointed-by queries implementation

std::vector<const Pointer*> PointerAnalysisQueries::getPointedBy(const MemoryObject* obj) const
{
    if (!obj) {
        return {};
    }
    
    std::vector<const Pointer*> result;
    
    // For each pointer managed by the pointer manager, check if it points to obj
    // Note: This is inefficient and could be optimized with a reverse points-to map
    for (auto& ptr : ptrManager.getAllPointers()) {
        if (mayPointTo(ptr, obj)) {
            result.push_back(ptr);
        }
    }
    
    return result;
}

std::vector<const llvm::Value*> PointerAnalysisQueries::getPointedByValues(const MemoryObject* obj) const
{
    if (!obj) {
        return {};
    }
    
    std::vector<const Pointer*> pointingPointers = getPointedBy(obj);
    std::unordered_set<const llvm::Value*> uniqueValues;
    
    // Extract unique values from pointers
    for (auto ptr : pointingPointers) {
        uniqueValues.insert(ptr->getValue());
    }
    
    return std::vector<const llvm::Value*>(uniqueValues.begin(), uniqueValues.end());
}

// Alias set queries implementation

std::vector<const Pointer*> PointerAnalysisQueries::getAliasSet(const Pointer* ptr) const
{
    if (!ptr) {
        return {};
    }
    
    std::vector<const Pointer*> result;
    
    // For each pointer, check if it may alias with ptr
    for (auto& otherPtr : ptrManager.getAllPointers()) {
        if (ptr != otherPtr && mayAlias(ptr, otherPtr)) {
            result.push_back(otherPtr);
        }
    }
    
    return result;
}

std::vector<const llvm::Value*> PointerAnalysisQueries::getAliasSetValues(const llvm::Value* val) const
{
    if (!val) {
        return {};
    }
    
    std::unordered_set<const llvm::Value*> uniqueValues;
    auto ptrs = ptrManager.getPointersWithValue(val->stripPointerCasts());
    
    for (auto ptr : ptrs) {
        auto aliasSet = getAliasSet(ptr);
        
        for (auto aliasPtr : aliasSet) {
            uniqueValues.insert(aliasPtr->getValue());
        }
    }
    
    return std::vector<const llvm::Value*>(uniqueValues.begin(), uniqueValues.end());
}

std::vector<std::pair<const Pointer*, const Pointer*>> PointerAnalysisQueries::getAllAliasingPairs(
    const std::vector<const Pointer*>& pointers) const
{
    std::vector<std::pair<const Pointer*, const Pointer*>> result;
    
    // Check all pairs of pointers
    for (size_t i = 0; i < pointers.size(); ++i) {
        for (size_t j = i + 1; j < pointers.size(); ++j) {
            if (mayAlias(pointers[i], pointers[j])) {
                result.emplace_back(pointers[i], pointers[j]);
            }
        }
    }
    
    return result;
}

} // namespace tpa 