/*
 * IntrospectiveSelectiveKCFA.cpp

 https://yanniss.github.io/introspective-pldi14.pdf#page=5.35
 *
 * Implementation of the Introspective Selective k-CFA approach that uses
 * heuristics based on context-insensitive pre-analysis to determine
 * which call sites and allocation sites to refine.
 */

#include "Alias/FSCS/Context/IntrospectiveSelectiveKCFA.h"
#include "Alias/FSCS/Context/ProgramPoint.h"
#include "Alias/FSCS/MemoryModel/MemoryObject.h"
#include "Alias/FSCS/MemoryModel/Pointer.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/CallSite.h>
#include <algorithm>

using namespace llvm;

namespace context
{

// Initialize static members
std::unordered_map<const llvm::Instruction*, unsigned> IntrospectiveSelectiveKCFA::callSiteInFlow;
std::unordered_map<const llvm::Function*, unsigned> IntrospectiveSelectiveKCFA::methodTotalPointsToVolume;
std::unordered_map<const llvm::Function*, unsigned> IntrospectiveSelectiveKCFA::methodMaxVarPointsTo;
std::unordered_map<const llvm::Function*, unsigned> IntrospectiveSelectiveKCFA::methodMaxVarFieldPointsTo;

std::unordered_map<const llvm::Instruction*, unsigned> IntrospectiveSelectiveKCFA::allocSiteMaxFieldPointsTo;
std::unordered_map<const llvm::Instruction*, unsigned> IntrospectiveSelectiveKCFA::allocSiteTotalFieldPointsTo;
std::unordered_map<const llvm::Instruction*, unsigned> IntrospectiveSelectiveKCFA::allocSitePointedByVars;
std::unordered_map<const llvm::Instruction*, unsigned> IntrospectiveSelectiveKCFA::allocSitePointedByObjs;

unsigned IntrospectiveSelectiveKCFA::thresholdPointedByVars = 50;
unsigned IntrospectiveSelectiveKCFA::thresholdInFlow = 100;
unsigned IntrospectiveSelectiveKCFA::thresholdMaxVarFieldPointsTo = 75;
unsigned IntrospectiveSelectiveKCFA::thresholdTotalPointsToVolume = 200;
unsigned IntrospectiveSelectiveKCFA::thresholdFieldPtsMultipliedByVars = 5000;
bool IntrospectiveSelectiveKCFA::useHeuristicA = true;

// Helper function to identify allocation sites
static bool isAllocationSite(const Instruction* inst)
{
    if (const CallInst* callInst = dyn_cast<CallInst>(inst)) {
        if (const Function* callee = callInst->getCalledFunction()) {
            StringRef name = callee->getName();
            return (name == "malloc" || name == "calloc" || name == "realloc" || 
                    name == "_Znwm" || name == "_Znam" ||   // new, new[] on 64-bit
                    name == "_Znwj" || name == "_Znaj");    // new, new[] on 32-bit
        }
    }
    return false;
}

// Helper to get function from call/invoke instruction
static const Function* getCalledFunction(const Instruction* inst)
{
    if (const CallInst* callInst = dyn_cast<CallInst>(inst)) {
        return callInst->getCalledFunction();
    }
    else if (const InvokeInst* invokeInst = dyn_cast<InvokeInst>(inst)) {
        return invokeInst->getCalledFunction();
    }
    return nullptr;
}

unsigned IntrospectiveSelectiveKCFA::computeCallSiteInFlow(
    const Instruction* callSite, const tpa::PointerAnalysisQueries& queries)
{
    unsigned totalSize = 0;
    
    if (const CallInst* call = dyn_cast<CallInst>(callSite)) {
        // Sum up points-to set sizes for all pointer arguments
        for (unsigned i = 0; i < call->getNumArgOperands(); ++i) {
            const Value* arg = call->getArgOperand(i);
            if (arg->getType()->isPointerTy()) {
                tpa::PtsSet pts = queries.getPointsToSet(arg);
                totalSize += pts.size();
            }
        }
    } 
    else if (const InvokeInst* invoke = dyn_cast<InvokeInst>(callSite)) {
        // Same logic for invoke instructions
        for (unsigned i = 0; i < invoke->getNumArgOperands(); ++i) {
            const Value* arg = invoke->getArgOperand(i);
            if (arg->getType()->isPointerTy()) {
                tpa::PtsSet pts = queries.getPointsToSet(arg);
                totalSize += pts.size();
            }
        }
    }
    
    return totalSize;
}

unsigned IntrospectiveSelectiveKCFA::computeMethodTotalPointsToVolume(
    const Function* function, const tpa::PointerAnalysisQueries& queries)
{
    unsigned totalVolume = 0;
    
    // Go through all local variables in the function
    for (const_inst_iterator I = inst_begin(function), E = inst_end(function); I != E; ++I) {
        const Instruction* inst = &*I;
        
        // Only consider instructions that define pointers
        if (inst->getType()->isPointerTy()) {
            tpa::PtsSet pts = queries.getPointsToSet(inst);
            totalVolume += pts.size();
        }
        
        // Also include pointer operands
        for (unsigned i = 0; i < inst->getNumOperands(); ++i) {
            const Value* op = inst->getOperand(i);
            if (op->getType()->isPointerTy()) {
                // Only count local variables in the function
                if (isa<Instruction>(op) || isa<Argument>(op)) {
                    tpa::PtsSet pts = queries.getPointsToSet(op);
                    totalVolume += pts.size();
                }
            }
        }
    }
    
    // Also include function arguments
    for (Function::const_arg_iterator AI = function->arg_begin(), AE = function->arg_end(); 
         AI != AE; ++AI) {
        const Argument* arg = &*AI;
        if (arg->getType()->isPointerTy()) {
            tpa::PtsSet pts = queries.getPointsToSet(arg);
            totalVolume += pts.size();
        }
    }
    
    return totalVolume;
}

unsigned IntrospectiveSelectiveKCFA::computeMethodMaxVarPointsTo(
    const Function* function, const tpa::PointerAnalysisQueries& queries)
{
    unsigned maxSize = 0;
    
    // Check local variables
    for (const_inst_iterator I = inst_begin(function), E = inst_end(function); I != E; ++I) {
        const Instruction* inst = &*I;
        if (inst->getType()->isPointerTy()) {
            tpa::PtsSet pts = queries.getPointsToSet(inst);
            maxSize = std::max(maxSize, static_cast<unsigned>(pts.size()));
        }
    }
    
    // Check arguments
    for (Function::const_arg_iterator AI = function->arg_begin(), AE = function->arg_end(); 
         AI != AE; ++AI) {
        const Argument* arg = &*AI;
        if (arg->getType()->isPointerTy()) {
            tpa::PtsSet pts = queries.getPointsToSet(arg);
            maxSize = std::max(maxSize, static_cast<unsigned>(pts.size()));
        }
    }
    
    return maxSize;
}

unsigned IntrospectiveSelectiveKCFA::computeAllocSiteMaxFieldPointsTo(
    const Instruction* allocSite, const tpa::PointerAnalysisQueries& queries)
{
    // This requires knowledge of the memory objects representing this allocation site
    // and their field structure. For simplicity, we'll estimate using a limited implementation.
    
    // First, find all memory objects that this allocation site could create
    tpa::PtsSet allocObjs = queries.getPointsToSet(allocSite);
    unsigned maxFieldPtsSize = 0;
    
    // For each object that might be allocated at this site
    for (const tpa::MemoryObject* obj : allocObjs) {
        // For each field (approximated as GEP instructions using this alloc site)
        // Find GEP instructions that use this allocated object
        for (const tpa::Pointer* ptr : queries.getPointedBy(obj)) {
            // Here we'd ideally inspect field points-to sets
            // As a simple approximation, we count the points-to set size for each pointer derived from this object
            const Value* val = ptr->getValue();
            if (isa<GetElementPtrInst>(val)) {
                tpa::PtsSet fieldPts = queries.getPointsToSet(val);
                maxFieldPtsSize = std::max(maxFieldPtsSize, static_cast<unsigned>(fieldPts.size()));
            }
        }
    }
    
    return maxFieldPtsSize;
}

unsigned IntrospectiveSelectiveKCFA::computeAllocSiteTotalFieldPointsTo(
    const Instruction* allocSite, const tpa::PointerAnalysisQueries& queries)
{
    // Similar to the max field method, but we sum up all field points-to set sizes
    tpa::PtsSet allocObjs = queries.getPointsToSet(allocSite);
    unsigned totalFieldPtsSize = 0;
    
    for (const tpa::MemoryObject* obj : allocObjs) {
        for (const tpa::Pointer* ptr : queries.getPointedBy(obj)) {
            const Value* val = ptr->getValue();
            if (isa<GetElementPtrInst>(val)) {
                tpa::PtsSet fieldPts = queries.getPointsToSet(val);
                totalFieldPtsSize += fieldPts.size();
            }
        }
    }
    
    return totalFieldPtsSize;
}

unsigned IntrospectiveSelectiveKCFA::computeMethodMaxVarFieldPointsTo(
    const Function* function, const tpa::PointerAnalysisQueries& queries)
{
    unsigned maxVarFieldPtsSize = 0;
    
    // Go through all local pointer variables
    for (const_inst_iterator I = inst_begin(function), E = inst_end(function); I != E; ++I) {
        const Instruction* inst = &*I;
        if (inst->getType()->isPointerTy()) {
            // For each object this variable points to
            tpa::PtsSet pts = queries.getPointsToSet(inst);
            for (const tpa::MemoryObject* obj : pts) {
                // Find the max field-points-to for this object
                unsigned maxFieldSize = 0;
                
                // For each pointer derived from this object (field accesses)
                for (const tpa::Pointer* ptr : queries.getPointedBy(obj)) {
                    const Value* val = ptr->getValue();
                    if (isa<GetElementPtrInst>(val)) {
                        tpa::PtsSet fieldPts = queries.getPointsToSet(val);
                        maxFieldSize = std::max(maxFieldSize, static_cast<unsigned>(fieldPts.size()));
                    }
                }
                
                maxVarFieldPtsSize = std::max(maxVarFieldPtsSize, maxFieldSize);
            }
        }
    }
    
    // Also check function arguments
    for (Function::const_arg_iterator AI = function->arg_begin(), AE = function->arg_end(); 
         AI != AE; ++AI) {
        const Argument* arg = &*AI;
        if (arg->getType()->isPointerTy()) {
            tpa::PtsSet pts = queries.getPointsToSet(arg);
            for (const tpa::MemoryObject* obj : pts) {
                unsigned maxFieldSize = 0;
                
                for (const tpa::Pointer* ptr : queries.getPointedBy(obj)) {
                    const Value* val = ptr->getValue();
                    if (isa<GetElementPtrInst>(val)) {
                        tpa::PtsSet fieldPts = queries.getPointsToSet(val);
                        maxFieldSize = std::max(maxFieldSize, static_cast<unsigned>(fieldPts.size()));
                    }
                }
                
                maxVarFieldPtsSize = std::max(maxVarFieldPtsSize, maxFieldSize);
            }
        }
    }
    
    return maxVarFieldPtsSize;
}

unsigned IntrospectiveSelectiveKCFA::computeAllocSitePointedByVars(
    const Instruction* allocSite, const tpa::PointerAnalysisQueries& queries)
{
    // Count unique local variables that point to objects created at this allocation site
    tpa::PtsSet allocObjs = queries.getPointsToSet(allocSite);
    std::unordered_set<const llvm::Value*> pointingVars;
    
    for (const tpa::MemoryObject* obj : allocObjs) {
        std::vector<const llvm::Value*> pointedBy = queries.getPointedByValues(obj);
        pointingVars.insert(pointedBy.begin(), pointedBy.end());
    }
    
    return static_cast<unsigned>(pointingVars.size());
}

unsigned IntrospectiveSelectiveKCFA::computeAllocSitePointedByObjs(
    const Instruction* allocSite, const tpa::PointerAnalysisQueries& queries)
{
    // Count unique (object, field) pairs that point to this allocation site
    tpa::PtsSet allocObjs = queries.getPointsToSet(allocSite);
    unsigned numPointedBy = 0;
    
    for (const tpa::MemoryObject* obj : allocObjs) {
        // In this simplified implementation, we count each pointer that points to this object
        // In a more detailed implementation, we'd specifically count (object, field) pairs
        std::vector<const tpa::Pointer*> pointedBy = queries.getPointedBy(obj);
        
        // Filter to only include GEP instructions (field accesses)
        for (const tpa::Pointer* ptr : pointedBy) {
            if (isa<GetElementPtrInst>(ptr->getValue())) {
                numPointedBy++;
            }
        }
    }
    
    return numPointedBy;
}

void IntrospectiveSelectiveKCFA::computeMetricsFromPreAnalysis(
    const tpa::PointerAnalysisQueries& queries, const llvm::Module& module)
{
    // Clear existing metrics
    callSiteInFlow.clear();
    methodTotalPointsToVolume.clear();
    methodMaxVarPointsTo.clear();
    methodMaxVarFieldPointsTo.clear();
    allocSiteMaxFieldPointsTo.clear();
    allocSiteTotalFieldPointsTo.clear();
    allocSitePointedByVars.clear();
    allocSitePointedByObjs.clear();
    
    // Process all functions in the module
    for (auto& F : module) {
        if (F.isDeclaration()) continue;
        
        // Compute function-level metrics
        methodTotalPointsToVolume[&F] = computeMethodTotalPointsToVolume(&F, queries);
        methodMaxVarPointsTo[&F] = computeMethodMaxVarPointsTo(&F, queries);
        methodMaxVarFieldPointsTo[&F] = computeMethodMaxVarFieldPointsTo(&F, queries);
        
        // Process instructions within the function
        for (const_inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {
            const Instruction* inst = &*I;
            
            // Compute call site metrics
            if (isa<CallInst>(inst) || isa<InvokeInst>(inst)) {
                callSiteInFlow[inst] = computeCallSiteInFlow(inst, queries);
            }
            
            // Compute allocation site metrics
            if (isAllocationSite(inst)) {
                allocSiteMaxFieldPointsTo[inst] = computeAllocSiteMaxFieldPointsTo(inst, queries);
                allocSiteTotalFieldPointsTo[inst] = computeAllocSiteTotalFieldPointsTo(inst, queries);
                allocSitePointedByVars[inst] = computeAllocSitePointedByVars(inst, queries);
                allocSitePointedByObjs[inst] = computeAllocSitePointedByObjs(inst, queries);
            }
        }
    }
}

void IntrospectiveSelectiveKCFA::applyHeuristics()
{
    if (useHeuristicA) {
        // Heuristic A: 
        // Refine all allocation sites except those with pointed-by-vars > K
        // Refine all call sites except those with in-flow > L or max-var-field-points-to > M
        
        // Process allocation sites
        for (const auto& pair : allocSitePointedByVars) {
            const Instruction* allocSite = pair.first;
            unsigned pointedByVars = pair.second;
            
            if (pointedByVars <= thresholdPointedByVars) {
                // Refine this allocation site (use default k=1)
                SelectiveKCFA::setAllocSiteLimit(allocSite, 1);
            } else {
                // Don't refine this allocation site (k=0)
                SelectiveKCFA::setAllocSiteLimit(allocSite, 0);
            }
        }
        
        // Process call sites
        for (const auto& pair : callSiteInFlow) {
            const Instruction* callSite = pair.first;
            unsigned inFlow = pair.second;
            
            // Get the called function for this call site
            const Function* calledFunction = getCalledFunction(callSite);
            
            // Skip indirect calls where we can't determine the called function
            if (!calledFunction || calledFunction->isDeclaration()) {
                continue;
            }
            
            // Check if the function has a max var-field points-to metric
            auto varFieldIter = methodMaxVarFieldPointsTo.find(calledFunction);
            unsigned maxVarFieldPts = (varFieldIter != methodMaxVarFieldPointsTo.end()) 
                                     ? varFieldIter->second : 0;
            
            if (inFlow > thresholdInFlow || maxVarFieldPts > thresholdMaxVarFieldPointsTo) {
                // Don't refine this call site (k=0)
                SelectiveKCFA::setCallSiteLimit(callSite, 0);
            } else {
                // Refine this call site (use default k=1)
                SelectiveKCFA::setCallSiteLimit(callSite, 1);
            }
        }
    } else {
        // Heuristic B:
        // Refine all method calls except those with total-points-to-volume > P
        // Refine all object allocations except those with (total-field-points-to * pointed-by-vars) > Q
        
        // Process methods (call sites)
        for (const auto& pair : callSiteInFlow) {
            const Instruction* callSite = pair.first;
            
            // Get the called function for this call site
            const Function* calledFunction = getCalledFunction(callSite);
            
            // Skip indirect calls where we can't determine the called function
            if (!calledFunction || calledFunction->isDeclaration()) {
                continue;
            }
            
            // Check if function has a total points-to volume metric
            auto volIter = methodTotalPointsToVolume.find(calledFunction);
            unsigned totalVolume = (volIter != methodTotalPointsToVolume.end()) 
                                  ? volIter->second : 0;
            
            if (totalVolume > thresholdTotalPointsToVolume) {
                // Don't refine this call site (k=0)
                SelectiveKCFA::setCallSiteLimit(callSite, 0);
            } else {
                // Refine this call site (use default k=1)
                SelectiveKCFA::setCallSiteLimit(callSite, 1);
            }
        }
        
        // Process allocation sites
        for (const auto& pair : allocSiteTotalFieldPointsTo) {
            const Instruction* allocSite = pair.first;
            unsigned totalFieldPts = pair.second;
            
            // Get the pointed-by-vars metric for this allocation site
            auto varsIter = allocSitePointedByVars.find(allocSite);
            unsigned pointedByVars = (varsIter != allocSitePointedByVars.end()) 
                                    ? varsIter->second : 0;
            
            // Calculate the product
            unsigned product = totalFieldPts * pointedByVars;
            
            if (product > thresholdFieldPtsMultipliedByVars) {
                // Don't refine this allocation site (k=0)
                SelectiveKCFA::setAllocSiteLimit(allocSite, 0);
            } else {
                // Refine this allocation site (use default k=1)
                SelectiveKCFA::setAllocSiteLimit(allocSite, 1);
            }
        }
    }
}

void IntrospectiveSelectiveKCFA::printMetricsStats(llvm::raw_ostream& os)
{
    os << "IntrospectiveSelectiveKCFA Metrics Statistics:\n";
    os << "  Current heuristic: " << (useHeuristicA ? "A" : "B") << "\n\n";
    
    if (useHeuristicA) {
        os << "  Heuristic A thresholds:\n";
        os << "    Pointed-by-vars (K): " << thresholdPointedByVars << "\n";
        os << "    In-flow (L): " << thresholdInFlow << "\n";
        os << "    Max-var-field-points-to (M): " << thresholdMaxVarFieldPointsTo << "\n";
    } else {
        os << "  Heuristic B thresholds:\n";
        os << "    Total-points-to-volume (P): " << thresholdTotalPointsToVolume << "\n";
        os << "    Field-points-to * pointed-by-vars (Q): " << thresholdFieldPtsMultipliedByVars << "\n";
    }
    
    os << "\n  Call site metrics:\n";
    unsigned numCallSites = callSiteInFlow.size();
    unsigned numFilteredCallSites = 0;
    
    if (useHeuristicA) {
        // Count filtered call sites according to Heuristic A
        for (const auto& pair : callSiteInFlow) {
            const Instruction* callSite = pair.first;
            unsigned inFlow = pair.second;
            
            const Function* calledFunction = getCalledFunction(callSite);
            if (!calledFunction || calledFunction->isDeclaration()) continue;
            
            auto varFieldIter = methodMaxVarFieldPointsTo.find(calledFunction);
            unsigned maxVarFieldPts = (varFieldIter != methodMaxVarFieldPointsTo.end()) 
                                     ? varFieldIter->second : 0;
            
            if (inFlow > thresholdInFlow || maxVarFieldPts > thresholdMaxVarFieldPointsTo) {
                numFilteredCallSites++;
            }
        }
    } else {
        // Count filtered call sites according to Heuristic B
        for (const auto& pair : callSiteInFlow) {
            const Instruction* callSite = pair.first;
            
            const Function* calledFunction = getCalledFunction(callSite);
            if (!calledFunction || calledFunction->isDeclaration()) continue;
            
            auto volIter = methodTotalPointsToVolume.find(calledFunction);
            unsigned totalVolume = (volIter != methodTotalPointsToVolume.end()) 
                                  ? volIter->second : 0;
            
            if (totalVolume > thresholdTotalPointsToVolume) {
                numFilteredCallSites++;
            }
        }
    }
    
    os << "    Total call sites: " << numCallSites << "\n";
    os << "    Filtered call sites: " << numFilteredCallSites 
       << " (" << (numCallSites > 0 ? (100.0 * numFilteredCallSites / numCallSites) : 0.0) << "%)\n";
    
    os << "\n  Allocation site metrics:\n";
    unsigned numAllocSites = allocSitePointedByVars.size();
    unsigned numFilteredAllocSites = 0;
    
    if (useHeuristicA) {
        // Count filtered allocation sites according to Heuristic A
        for (const auto& pair : allocSitePointedByVars) {
            unsigned pointedByVars = pair.second;
            
            if (pointedByVars > thresholdPointedByVars) {
                numFilteredAllocSites++;
            }
        }
    } else {
        // Count filtered allocation sites according to Heuristic B
        for (const auto& pair : allocSiteTotalFieldPointsTo) {
            const Instruction* allocSite = pair.first;
            unsigned totalFieldPts = pair.second;
            
            auto varsIter = allocSitePointedByVars.find(allocSite);
            unsigned pointedByVars = (varsIter != allocSitePointedByVars.end()) 
                                    ? varsIter->second : 0;
            
            unsigned product = totalFieldPts * pointedByVars;
            
            if (product > thresholdFieldPtsMultipliedByVars) {
                numFilteredAllocSites++;
            }
        }
    }
    
    os << "    Total allocation sites: " << numAllocSites << "\n";
    os << "    Filtered allocation sites: " << numFilteredAllocSites 
       << " (" << (numAllocSites > 0 ? (100.0 * numFilteredAllocSites / numAllocSites) : 0.0) << "%)\n";
    
    // Print metric distributions
    os << "\n  Metric distributions:\n";
    
    // In-flow distribution
    if (!callSiteInFlow.empty()) {
        std::vector<unsigned> inFlowValues;
        for (const auto& pair : callSiteInFlow) {
            inFlowValues.push_back(pair.second);
        }
        std::sort(inFlowValues.begin(), inFlowValues.end());
        
        os << "    In-flow: min=" << inFlowValues.front() 
           << ", median=" << inFlowValues[inFlowValues.size()/2]
           << ", max=" << inFlowValues.back() << "\n";
    }
    
    // Method total points-to volume distribution
    if (!methodTotalPointsToVolume.empty()) {
        std::vector<unsigned> volumeValues;
        for (const auto& pair : methodTotalPointsToVolume) {
            volumeValues.push_back(pair.second);
        }
        std::sort(volumeValues.begin(), volumeValues.end());
        
        os << "    Method total points-to volume: min=" << volumeValues.front() 
           << ", median=" << volumeValues[volumeValues.size()/2]
           << ", max=" << volumeValues.back() << "\n";
    }
    
    // Allocation site pointed-by-vars distribution
    if (!allocSitePointedByVars.empty()) {
        std::vector<unsigned> pointedByValues;
        for (const auto& pair : allocSitePointedByVars) {
            pointedByValues.push_back(pair.second);
        }
        std::sort(pointedByValues.begin(), pointedByValues.end());
        
        os << "    Allocation site pointed-by-vars: min=" << pointedByValues.front() 
           << ", median=" << pointedByValues[pointedByValues.size()/2]
           << ", max=" << pointedByValues.back() << "\n";
    }
}

} // namespace context 