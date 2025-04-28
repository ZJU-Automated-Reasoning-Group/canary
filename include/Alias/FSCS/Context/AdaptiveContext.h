#pragma once

#include "Alias/FSCS/Context/Context.h"
#include "Alias/FSCS/Context/ProgramPoint.h"

#include <unordered_set>

namespace context
{

/**
 * @class AdaptiveContext
 * @brief Provides an adaptive context-sensitivity policy.
 *
 * This class implements a context-sensitivity policy that adaptively
 * decides which call sites should be tracked based on program behavior.
 * Rather than using a fixed k-limit, it selectively tracks specific call
 * sites that are deemed important for analysis precision.
 */
class AdaptiveContext
{
private:
	static std::unordered_set<ProgramPoint> trackedCallsites;  ///< Set of call sites being tracked
public:
	/**
	 * @brief Marks a call site to be tracked for context sensitivity
	 * @param pp The program point (context + instruction) to track
	 */
	static void trackCallSite(const ProgramPoint& pp);

	/**
	 * @brief Creates a new context by pushing a call site, but only if it's tracked
	 * @param ctx The existing context
	 * @param inst The call instruction to push
	 * @return A pointer to the new (or existing) context
	 */
	static const Context* pushContext(const Context* ctx, const llvm::Instruction* inst);
	
	/**
	 * @brief Creates a new context by pushing a call site from a program point
	 * @param pp The program point containing context and instruction
	 * @return A pointer to the new (or existing) context
	 */
	static const Context* pushContext(const ProgramPoint& pp);
};

}
