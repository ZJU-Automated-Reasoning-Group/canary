#pragma once

#include "Alias/FSCS/Context/Context.h"

namespace llvm
{
	class Instruction;
}

namespace context
{

class ProgramPoint;

/**
 * @class KLimitContext
 * @brief Provides a k-limited context-sensitivity policy.
 *
 * This class implements a context-sensitivity policy that limits the call stack depth
 * to a configurable value k. When the stack exceeds this limit, the oldest call sites
 * are dropped. This provides a way to balance precision and performance in context-sensitive
 * analyses by controlling the maximum context depth.
 */
class KLimitContext
{
private:
	static unsigned defaultLimit;  ///< The global k-limit for all contexts
public:
	/**
	 * @brief Sets the global k-limit for context sensitivity
	 * @param k The maximum call stack depth to maintain
	 */
	static void setLimit(unsigned k) { defaultLimit = k; }
	
	/**
	 * @brief Gets the current global k-limit
	 * @return The maximum call stack depth being maintained
	 */
	static unsigned getLimit() { return defaultLimit; }

	/**
	 * @brief Creates a new context by pushing a call site onto an existing context,
	 *        respecting the k-limit by dropping oldest call sites if needed
	 * @param ctx The existing context
	 * @param inst The call instruction to push
	 * @return A pointer to the new (or existing) context
	 */
	static const Context* pushContext(const Context* ctx, const llvm::Instruction* inst);
	
	/**
	 * @brief Creates a new context by pushing a call site from a ProgramPoint,
	 *        respecting the k-limit
	 * @param pp The program point containing context and instruction
	 * @return A pointer to the new (or existing) context
	 */
	static const Context* pushContext(const ProgramPoint& pp);
};

}
