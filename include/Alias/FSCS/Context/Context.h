#pragma once

#include "Support/ADT/Hashing.h"

#include <llvm/IR/Instruction.h>

#include <unordered_set>

namespace context
{

class ProgramPoint;

/**
 * @class Context
 * @brief Represents a calling context in the program as a stack of call sites.
 *
 * The Context class implements context-sensitivity for various analyses by
 * maintaining a linked chain of call sites that represent the call stack. This
 * enables distinguishing between different calling contexts of the same code.
 * Contexts are immutable and uniqued (interned) to save memory.
 */
class Context
{
private:
	const llvm::Instruction* callSite;  ///< The call instruction that created this context
	const Context* predContext;         ///< The predecessor (caller) context
	size_t sz;                          ///< The depth of this context in the call stack

	static std::unordered_set<Context> ctxSet;  ///< Global set for context uniquing/interning

	/**
	 * @brief Constructor for the global (empty) context
	 */
	Context(): callSite(nullptr), predContext(nullptr), sz(0) {}
	
	/**
	 * @brief Constructor for a non-global context
	 * @param c The call site instruction
	 * @param p The predecessor context
	 */
	Context(const llvm::Instruction* c, const Context* p): callSite(c), predContext(p), sz(p == nullptr ? 1 : p->sz + 1) {}
public:
	/**
	 * @brief Gets the call site instruction for this context
	 * @return The call instruction, or nullptr for the global context
	 */
	const llvm::Instruction* getCallSite() const { return callSite; }
	
	/**
	 * @brief Gets the size (depth) of this context
	 * @return The number of call sites in the context chain
	 */
	size_t size() const { return sz; }
	
	/**
	 * @brief Checks if this is the global (empty) context
	 * @return True if this is the global context, false otherwise
	 */
	bool isGlobalContext() const { return sz == 0; }

	/**
	 * @brief Equality comparison operator
	 * @param other The Context to compare with
	 * @return True if both contexts represent the same call stack
	 */
	bool operator==(const Context& other) const
	{
		return callSite == other.callSite && predContext == other.predContext;
	}
	
	/**
	 * @brief Inequality comparison operator
	 * @param other The Context to compare with
	 * @return True if the contexts represent different call stacks
	 */
	bool operator!=(const Context& other) const
	{
		return !(*this == other);
	}

	/**
	 * @brief Creates a new context by pushing a call site onto an existing context
	 * @param ctx The existing context (caller)
	 * @param inst The call instruction to push
	 * @return A pointer to the new (or existing) context
	 */
	static const Context* pushContext(const Context* ctx, const llvm::Instruction* inst);
	
	/**
	 * @brief Creates a new context by pushing a call site from a ProgramPoint
	 * @param pp The program point containing context and instruction
	 * @return A pointer to the new (or existing) context
	 */
	static const Context* pushContext(const ProgramPoint&);
	
	/**
	 * @brief Creates a new context by popping a call site from an existing context
	 * @param ctx The existing context
	 * @return A pointer to the predecessor context, or nullptr if at global context
	 */
	static const Context* popContext(const Context* ctx);
	
	/**
	 * @brief Gets the global (empty) context
	 * @return A pointer to the global context
	 */
	static const Context* getGlobalContext();

	/**
	 * @brief Gets all contexts that have been created
	 * @return A vector of pointers to all contexts
	 */
	static std::vector<const Context*> getAllContexts();
	
	/**
	 * @brief Friend declaration to allow the hash specialization to access private members
	 */
	friend struct std::hash<Context>;
};

}

namespace std
{
	/**
	 * @brief Hash function specialization for Context
	 *
	 * Enables Context to be used as a key in unordered containers
	 * by providing a hash function that combines the hashes of the
	 * call site and predecessor context.
	 */
	template<> struct hash<context::Context>
	{
		size_t operator()(const context::Context& c) const
		{
			return util::hashPair(c.callSite, c.predContext);
		}
	};
}
