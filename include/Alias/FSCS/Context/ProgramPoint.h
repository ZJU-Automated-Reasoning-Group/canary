#pragma once

#include "Support/ADT/Hashing.h"

namespace llvm
{
	class Instruction;
}

namespace context
{

class Context;

/**
 * @class ProgramPoint
 * @brief Represents a point in the program with its calling context.
 *
 * ProgramPoint combines an LLVM instruction with its context to uniquely
 * identify a specific execution point in the program. This is fundamental
 * for context-sensitive analyses, where the same instruction can be analyzed
 * differently depending on its calling context.
 */
class ProgramPoint
{
private:
	const Context* context;         ///< The calling context
	const llvm::Instruction* inst;  ///< The instruction

	using PairType = std::pair<const Context*, const llvm::Instruction*>;
public:
	/**
	 * @brief Constructs a ProgramPoint from a context and instruction
	 * @param c The calling context
	 * @param i The instruction
	 */
	ProgramPoint(const Context* c, const llvm::Instruction* i): context(c), inst(i) {}
	
	/**
	 * @brief Constructs a ProgramPoint from a pair of context and instruction
	 * @param p A pair containing the context and instruction
	 */
	ProgramPoint(const PairType& p): context(p.first), inst(p.second) {}

	/**
	 * @brief Gets the calling context
	 * @return The context pointer
	 */
	const Context* getContext() const { return context; }
	
	/**
	 * @brief Gets the instruction
	 * @return The instruction pointer
	 */
	const llvm::Instruction* getInstruction() const { return inst; }

	/**
	 * @brief Equality comparison operator
	 * @param other The ProgramPoint to compare with
	 * @return true if both context and instruction are equal
	 */
	bool operator==(const ProgramPoint& other) const
	{
		return (context == other.context) && (inst == other.inst);
	}
	
	/**
	 * @brief Inequality comparison operator
	 * @param other The ProgramPoint to compare with
	 * @return true if either context or instruction differs
	 */
	bool operator!=(const ProgramPoint& other) const
	{
		return !(*this == other);
	}
	
	/**
	 * @brief Less-than comparison operator for ordering
	 * @param rhs The right-hand side ProgramPoint to compare with
	 * @return true if this point should be ordered before rhs
	 */
	bool operator<(const ProgramPoint& rhs) const
	{
		if (context < rhs.context)
			return true;
		else if (rhs.context < context)
			return false;
		else
			return inst < rhs.inst;
	}
	
	/**
	 * @brief Greater-than comparison operator for ordering
	 * @param rhs The right-hand side ProgramPoint to compare with
	 * @return true if this point should be ordered after rhs
	 */
	bool operator>(const ProgramPoint& rhs) const
	{
		return rhs < *this;
	}
	
	/**
	 * @brief Less-than-or-equal comparison operator
	 * @param rhs The right-hand side ProgramPoint to compare with
	 * @return true if this point should be ordered before or equal to rhs
	 */
	bool operator<=(const ProgramPoint& rhs) const
	{
		return !(rhs < *this);
	}
	
	/**
	 * @brief Greater-than-or-equal comparison operator
	 * @param rhs The right-hand side ProgramPoint to compare with
	 * @return true if this point should be ordered after or equal to rhs
	 */
	bool operator>=(const ProgramPoint& rhs) const
	{
		return !(*this < rhs);
	}

	/**
	 * @brief Conversion operator to pair of context and instruction
	 * @return A pair containing the context and instruction
	 */
	operator PairType() const
	{
		return std::make_pair(context, inst);
	}
};

}

namespace std
{
	/**
	 * @brief Hash function specialization for ProgramPoint
	 *
	 * Enables ProgramPoint to be used as a key in unordered containers
	 * by providing a hash function that combines the hashes of the
	 * context and instruction pointers.
	 */
	template<> struct hash<context::ProgramPoint>
	{
		size_t operator()(const context::ProgramPoint& pp) const
		{
			return util::hashPair(pp.getContext(), pp.getInstruction());	
		}
	};
}
