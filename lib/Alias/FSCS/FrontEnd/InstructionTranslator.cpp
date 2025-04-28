#include "Alias/FSCS/Context/Context.h"
#include "Alias/FSCS/FrontEnd/CFG/InstructionTranslator.h"
#include "Alias/FSCS/FrontEnd/Type/TypeMap.h"
#include "Alias/FSCS/Program/CFG/CFG.h"

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace tpa
{

/**
 * @brief Creates a copy node for an instruction with multiple sources
 * 
 * @param inst The instruction representing the destination
 * @param srcs Set of source values
 * @return Pointer to the created CFG node
 * 
 * Copy nodes represent instructions that copy pointer values, such as PHI nodes
 * and select instructions. They propagate points-to information from sources
 * to the destination.
 */
tpa::CFGNode* InstructionTranslator::createCopyNode(const Instruction* inst, const SmallPtrSetImpl<const Value*>& srcs)
{
	assert(inst != nullptr && srcs.size() > 0u);
	auto srcVals = std::vector<const Value*>(srcs.begin(), srcs.end());
	return cfg.create<tpa::CopyCFGNode>(inst, std::move(srcVals));
}

/**
 * @brief Translates an LLVM AllocaInst to a pointer analysis CFG node
 * 
 * @param allocaInst The LLVM alloca instruction
 * @return Pointer to the created AllocCFGNode
 * 
 * Alloc nodes represent stack allocations and create new memory objects
 * in the points-to analysis. The allocated type is tracked to enable
 * field-sensitive analysis.
 */
tpa::CFGNode* InstructionTranslator::visitAllocaInst(AllocaInst& allocaInst)
{
	assert(allocaInst.getType()->isPointerTy());

	auto allocType = typeMap.lookup(allocaInst.getAllocatedType());
	assert(allocType != nullptr && "Alloc type not found");

	return cfg.create<tpa::AllocCFGNode>(&allocaInst, allocType);
}

/**
 * @brief Translates an LLVM LoadInst to a pointer analysis CFG node
 * 
 * @param loadInst The LLVM load instruction
 * @return Pointer to the created LoadCFGNode or nullptr if not a pointer load
 * 
 * Load nodes represent memory reads. In pointer analysis, they load pointers
 * from memory locations. If the loaded value is not a pointer, this returns nullptr
 * as non-pointer loads are irrelevant to points-to analysis.
 */
tpa::CFGNode* InstructionTranslator::visitLoadInst(LoadInst& loadInst)
{
	if (!loadInst.getType()->isPointerTy())
		return nullptr;

	auto dstVal = &loadInst;
	auto srcVal = loadInst.getPointerOperand()->stripPointerCasts();
	return cfg.create<tpa::LoadCFGNode>(dstVal, srcVal);
}

/**
 * @brief Translates an LLVM StoreInst to a pointer analysis CFG node
 * 
 * @param storeInst The LLVM store instruction
 * @return Pointer to the created StoreCFGNode or nullptr if not storing a pointer
 * 
 * Store nodes represent memory writes. In pointer analysis, they represent
 * storing a pointer value to a memory location. If the stored value is not
 * a pointer, this returns nullptr as non-pointer stores are irrelevant.
 */
tpa::CFGNode* InstructionTranslator::visitStoreInst(StoreInst& storeInst)
{
	auto valOp = storeInst.getValueOperand();
	if (!valOp->getType()->isPointerTy())
		return nullptr;
	auto ptrOp = storeInst.getPointerOperand();

	return cfg.create<tpa::StoreCFGNode>(ptrOp->stripPointerCasts(), valOp->stripPointerCasts());
}

/**
 * @brief Translates an LLVM ReturnInst to a pointer analysis CFG node
 * 
 * @param retInst The LLVM return instruction
 * @return Pointer to the created ReturnCFGNode
 * 
 * Return nodes mark the end of a function and propagate the return value
 * to the corresponding call site. The node is also registered as the exit
 * node for the current CFG.
 */
tpa::CFGNode* InstructionTranslator::visitReturnInst(ReturnInst& retInst)
{
	auto retVal = retInst.getReturnValue();
	if (retVal != nullptr)
		retVal = retVal->stripPointerCasts();

	auto retNode = cfg.create<tpa::ReturnCFGNode>(retVal);
	cfg.setExitNode(retNode);
	return retNode;
}

/**
 * @brief Translates an LLVM CallSite to a pointer analysis CFG node
 * 
 * @param cs The LLVM call site
 * @return Pointer to the created CallCFGNode
 * 
 * Call nodes represent function calls and connect the caller to potential
 * callees. They track pointer arguments to support interprocedural analysis.
 * The called value itself may be a function pointer requiring analysis.
 */
tpa::CFGNode* InstructionTranslator::visitCallSite(CallSite cs)
{
	auto funPtr = cs.getCalledValue()->stripPointerCasts();

	// The reinterpret_cast here just use the instruction pointer to assign a unique id to the corresponding call site
	auto callNode = cfg.create<tpa::CallCFGNode>(funPtr, cs.getInstruction());

	for (unsigned i = 0; i < cs.arg_size(); ++i)
	{
		auto arg = cs.getArgument(i)->stripPointerCasts();

		if (!arg->getType()->isPointerTy())
			continue;

		callNode->addArgument(arg);
	}
	return callNode;
}

/**
 * @brief Translates an LLVM PHINode to a pointer analysis CFG node
 * 
 * @param phiInst The LLVM PHI node
 * @return Pointer to the created CopyCFGNode or nullptr if not a pointer PHI
 * 
 * PHI nodes select values based on the predecessor basic block. In pointer
 * analysis, they are modeled as copy nodes from all incoming values to the
 * PHI result. Non-pointer PHIs are ignored.
 */
tpa::CFGNode* InstructionTranslator::visitPHINode(PHINode& phiInst)
{
	if (!phiInst.getType()->isPointerTy())
		return nullptr;

	auto srcs = SmallPtrSet<const Value*, 4>();
	for (unsigned i = 0; i < phiInst.getNumIncomingValues(); ++i)
	{
		auto value = phiInst.getIncomingValue(i)->stripPointerCasts();
		if (isa<UndefValue>(value))
			continue;
		srcs.insert(value);
	}

	return createCopyNode(&phiInst, srcs);
}

/**
 * @brief Translates an LLVM SelectInst to a pointer analysis CFG node
 * 
 * @param selectInst The LLVM select instruction
 * @return Pointer to the created CopyCFGNode or nullptr if not a pointer select
 * 
 * Select instructions choose between two values based on a condition.
 * In pointer analysis, they are modeled as copy nodes from both potential
 * sources to the result. Non-pointer selects are ignored.
 */
tpa::CFGNode* InstructionTranslator::visitSelectInst(SelectInst& selectInst)
{
	if (!selectInst.getType()->isPointerTy())
		return nullptr;

	auto srcs = SmallPtrSet<const Value*, 2>();
	srcs.insert(selectInst.getFalseValue()->stripPointerCasts());
	srcs.insert(selectInst.getTrueValue()->stripPointerCasts());
	
	return createCopyNode(&selectInst, srcs);
}

/**
 * @brief Translates an LLVM GetElementPtrInst to a pointer analysis CFG node
 * 
 * @param gepInst The LLVM GEP instruction
 * @return Pointer to the created OffsetCFGNode
 * 
 * GEP (GetElementPtr) instructions compute addresses within arrays and structs.
 * In pointer analysis, they are modeled as offset nodes with either constant
 * or variable offsets. This enables field-sensitive analysis of complex objects.
 */
tpa::CFGNode* InstructionTranslator::visitGetElementPtrInst(GetElementPtrInst& gepInst)
{
	assert(gepInst.getType()->isPointerTy());

	auto srcVal = gepInst.getPointerOperand()->stripPointerCasts();

	// Constant-offset GEP
	auto gepOffset = APInt(dataLayout.getPointerTypeSizeInBits(srcVal->getType()), 0);
	if (gepInst.accumulateConstantOffset(dataLayout, gepOffset))
	{
		auto offset = gepOffset.getSExtValue();

		return cfg.create<tpa::OffsetCFGNode>(&gepInst, srcVal, offset, false);
	}

	// Variable-offset GEP
	auto numOps = gepInst.getNumOperands();
	if (numOps != 2 && numOps != 3)
		llvm_unreachable("Found a non-canonicalized GEP. Please run -expand-gep pass first!");

	size_t offset = dataLayout.getPointerSize();
	if (numOps == 2)
		offset = dataLayout.getTypeAllocSize(cast<SequentialType>(srcVal->getType())->getElementType());
	else
	{
		assert(isa<ConstantInt>(gepInst.getOperand(1)) && cast<ConstantInt>(gepInst.getOperand(1))->isZero());
		auto elemType = cast<SequentialType>(gepInst.getPointerOperand()->getType())->getElementType();
		offset = dataLayout.getTypeAllocSize(cast<SequentialType>(elemType)->getElementType());
	}

	return cfg.create<tpa::OffsetCFGNode>(&gepInst, srcVal, offset, true);
}

/**
 * @brief Translates an LLVM IntToPtrInst to a pointer analysis CFG node
 * 
 * @param inst The LLVM inttoptr instruction
 * @return Pointer to the created CopyCFGNode
 * 
 * IntToPtr instructions convert integers to pointers. In pointer analysis,
 * these are conservatively handled by assigning the universal pointer set
 * to the result, as the exact pointer target cannot be statically determined.
 */
tpa::CFGNode* InstructionTranslator::visitIntToPtrInst(IntToPtrInst& inst)
{
	assert(inst.getType()->isPointerTy());

	// Common cases are handled in FoldIntToPtr prepass. In other cases, we make no effort to track what the rhs is. Assign UniversalPtr to the rhs
	std::vector<const llvm::Value*> srcs = { UndefValue::get(inst.getType()) };
	return cfg.create<tpa::CopyCFGNode>(&inst, std::move(srcs));
}

/**
 * @brief Translates an LLVM BitCastInst to a pointer analysis CFG node
 * 
 * @param bcInst The LLVM bitcast instruction
 * @return nullptr (bitcasts are handled elsewhere)
 * 
 * Bitcast operations are generally irrelevant to pointer analysis as they
 * don't change the target address, just its type. These are typically
 * handled directly by stripping pointer casts during other operations.
 */
tpa::CFGNode* InstructionTranslator::visitBitCastInst(BitCastInst& bcInst)
{
	return nullptr;
}

/**
 * @brief Handles unsupported LLVM instructions
 * 
 * @param inst The unsupported instruction
 * @return nullptr (indicating no effect on pointer analysis)
 * 
 * This is a fallback handler for instructions not explicitly supported
 * by the pointer analysis. It prints a warning and treats the instruction
 * as a no-op, which may reduce precision but maintains soundness.
 */
tpa::CFGNode* InstructionTranslator::handleUnsupportedInst(const Instruction& inst)
{
	errs() << "Warning: Unsupported instruction encountered: " << inst << "\n";
	errs() << "Treating as no-op. Analysis results may be less precise.\n";
	return nullptr;
}

/**
 * @brief Translates an LLVM ExtractValueInst to a pointer analysis CFG node
 * 
 * @param inst The LLVM extractvalue instruction
 * @return Result of handleUnsupportedInst or nullptr if not extracting a pointer
 */
tpa::CFGNode* InstructionTranslator::visitExtractValueInst(ExtractValueInst& inst)
{
	if (!inst.getType()->isPointerTy())
		return nullptr;
	return handleUnsupportedInst(inst);
}

/**
 * @brief Translates an LLVM InsertValueInst to a pointer analysis CFG node
 * 
 * @param inst The LLVM insertvalue instruction
 * @return Result of handleUnsupportedInst or nullptr if not inserting a pointer
 */
tpa::CFGNode* InstructionTranslator::visitInsertValueInst(InsertValueInst& inst)
{
	if (!inst.getType()->isPointerTy())
		return nullptr;
	return handleUnsupportedInst(inst);
}

/**
 * @brief Translates an LLVM VAArgInst to a pointer analysis CFG node
 * 
 * @param inst The LLVM va_arg instruction
 * @return Result of handleUnsupportedInst
 */
tpa::CFGNode* InstructionTranslator::visitVAArgInst(VAArgInst& inst)
{
	return handleUnsupportedInst(inst);
}

/**
 * @brief Translates an LLVM ExtractElementInst to a pointer analysis CFG node
 * 
 * @param inst The LLVM extractelement instruction
 * @return Result of handleUnsupportedInst
 */
tpa::CFGNode* InstructionTranslator::visitExtractElementInst(ExtractElementInst& inst)
{
	return handleUnsupportedInst(inst);
}

/**
 * @brief Translates an LLVM InsertElementInst to a pointer analysis CFG node
 * 
 * @param inst The LLVM insertelement instruction
 * @return Result of handleUnsupportedInst
 */
tpa::CFGNode* InstructionTranslator::visitInsertElementInst(InsertElementInst& inst)
{
	return handleUnsupportedInst(inst);
}

/**
 * @brief Translates an LLVM ShuffleVectorInst to a pointer analysis CFG node
 * 
 * @param inst The LLVM shufflevector instruction
 * @return Result of handleUnsupportedInst
 */
tpa::CFGNode* InstructionTranslator::visitShuffleVectorInst(ShuffleVectorInst& inst)
{
	return handleUnsupportedInst(inst);
}

/**
 * @brief Translates an LLVM LandingPadInst to a pointer analysis CFG node
 * 
 * @param inst The LLVM landingpad instruction
 * @return Result of handleUnsupportedInst
 */
tpa::CFGNode* InstructionTranslator::visitLandingPadInst(LandingPadInst& inst)
{
	return handleUnsupportedInst(inst);
}

}