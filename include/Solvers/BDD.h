/**
 * @file BDD.h
 * @brief A user-friendly C++ wrapper for CUDD Binary Decision Diagrams
 */

#ifndef BDD_H
#define BDD_H

#include <cstdio>  // For FILE type
#include "Solvers/CUDD/cudd.h"
#include <vector>
#include <string>
#include <functional>
#include <stdexcept>

namespace Solvers {

/**
 * @class BDD
 * @brief A user-friendly wrapper around CUDD's Binary Decision Diagram functionality
 *
 * This class provides a more intuitive C++ interface for working with BDDs,
 * hiding the complexity of the CUDD library and allowing for easier manipulation
 * of Binary Decision Diagrams.
 */
class BDD {
public:
    /**
     * @brief Constructs a BDD manager
     * @param numVars Initial number of variables
     * @param numVarsZ Initial number of ZDD variables (default: 0)
     * @param numSlots Initial size of unique tables (default: CUDD_UNIQUE_SLOTS)
     * @param cacheSize Initial size of cache (default: CUDD_CACHE_SLOTS)
     * @param maxMemory Maximum memory in bytes (default: 0 = unlimited)
     */
    BDD(unsigned int numVars, 
        unsigned int numVarsZ = 0, 
        unsigned int numSlots = CUDD_UNIQUE_SLOTS, 
        unsigned int cacheSize = CUDD_CACHE_SLOTS, 
        unsigned long maxMemory = 0);
    
    /**
     * @brief Destructor - cleans up the BDD manager
     */
    ~BDD();
    
    // Disable copy to prevent accidental sharing of manager
    BDD(const BDD&) = delete;
    BDD& operator=(const BDD&) = delete;
    
    // Allow move semantics
    BDD(BDD&& other) noexcept;
    BDD& operator=(BDD&& other) noexcept;
    
    /**
     * @brief Represents a BDD node with convenient operators
     */
    class Node {
    public:
        // Default constructor creates a null node
        Node() : manager(nullptr), node(nullptr) {}
        
        // Constructor from a DdNode
        Node(DdManager* mgr, DdNode* n);
        
        // Destructor decreases reference count
        ~Node();
        
        // Copy constructor increases reference count
        Node(const Node& other);
        
        // Move constructor
        Node(Node&& other) noexcept;
        
        // Copy assignment increases reference count
        Node& operator=(const Node& other);
        
        // Move assignment
        Node& operator=(Node&& other) noexcept;
        
        // Logical operators
        Node operator&(const Node& other) const;
        Node operator|(const Node& other) const;
        Node operator^(const Node& other) const;
        Node operator!() const;
        Node operator~() const { return !(*this); }
        
        // Implication
        Node implies(const Node& other) const;
        
        // Equivalence (XNOR)
        Node iff(const Node& other) const;
        
        // Existential quantification
        Node existsQuantify(const Node& vars) const;
        
        // Universal quantification
        Node universalQuantify(const Node& vars) const;
        
        // Substitute one variable with another node
        Node substitute(int varIndex, const Node& replacement) const;
        
        // Check if this node is a tautology
        bool isTautology() const;
        
        // Check if this node is satisfiable
        bool isSatisfiable() const;
        
        // Check if this node represents the constant false
        bool isZero() const;
        
        // Check if this node represents the constant true
        bool isOne() const;
        
        // Get the variable index at the root of this node
        int getIndex() const;
        
        // Get the Then-branch of this node
        Node getThen() const;
        
        // Get the Else-branch of this node
        Node getElse() const;
        
        // Get a satisfying assignment as a vector of booleans
        std::vector<bool> getSatisfyingAssignment() const;
        
        // Get all satisfying assignments
        std::vector<std::vector<bool>> getAllSatisfyingAssignments() const;
        
        // Count the number of satisfying assignments
        double countSatisfyingAssignments(int numVars) const;
        
        // Get the support variables (indices of variables this node depends on)
        std::vector<int> getSupportVariables() const;
        
        // Get the number of nodes in this BDD
        int getNodeCount() const;
        
        // Print the BDD as a dot file for visualization
        void printDot(const std::string& filename, 
                      const std::vector<std::string>& varNames = {}) const;
        
        // Access the internal DdNode (for advanced users)
        DdNode* getInternalNode() const { return node; }
        
        // Access the manager (for advanced users)
        DdManager* getManager() const { return manager; }
        
    private:
        DdManager* manager;
        DdNode* node;
        
        friend class BDD;
    };
    
    /**
     * @brief Create a variable node
     * @param index The index of the variable
     * @return A Node representing the variable
     */
    Node makeVar(int index);
    
    /**
     * @brief Create a negated variable node
     * @param index The index of the variable
     * @return A Node representing the negated variable
     */
    Node makeNotVar(int index);
    
    /**
     * @brief Create a constant true node
     * @return A Node representing the constant true
     */
    Node makeOne();
    
    /**
     * @brief Create a constant false node
     * @return A Node representing the constant false
     */
    Node makeZero();
    
    /**
     * @brief Create a node representing a conjunction of variables
     * @param vars Vector of variable indices
     * @param phase Vector of variable phases (true for positive, false for negative)
     * @return A Node representing the conjunction
     */
    Node makeCube(const std::vector<int>& vars, const std::vector<bool>& phase);
    
    /**
     * @brief Create a node from a boolean function
     * @param func The boolean function to encode
     * @param numVars Number of variables in the domain
     * @return A Node representing the function
     */
    Node makeFromFunction(const std::function<bool(const std::vector<bool>&)>& func, int numVars);
    
    /**
     * @brief Create a node from a truth table
     * @param truthTable Vector of function outputs for each input combination
     * @param numVars Number of variables (must satisfy 2^numVars == truthTable.size())
     * @return A Node representing the function
     */
    Node makeFromTruthTable(const std::vector<bool>& truthTable, int numVars);
    
    /**
     * @brief Create a node representing an equality comparison (a == b)
     * @param aVars Indices of variables for the first operand
     * @param bVars Indices of variables for the second operand
     * @return A Node representing the equality comparison
     */
    Node makeEquality(const std::vector<int>& aVars, const std::vector<int>& bVars);
    
    /**
     * @brief Create a node representing a less-than comparison (a < b)
     * @param aVars Indices of variables for the first operand
     * @param bVars Indices of variables for the second operand
     * @return A Node representing the less-than comparison
     */
    Node makeLessThan(const std::vector<int>& aVars, const std::vector<int>& bVars);
    
    /**
     * @brief Enable dynamic variable reordering
     * @param method The reordering method to use
     */
    void enableReordering(Cudd_ReorderingType method = CUDD_REORDER_SIFT);
    
    /**
     * @brief Disable dynamic variable reordering
     */
    void disableReordering();
    
    /**
     * @brief Force a reordering of the variables
     * @param method The reordering method to use
     * @return True if successful, false otherwise
     */
    bool reorderVariables(Cudd_ReorderingType method = CUDD_REORDER_SIFT);
    
    /**
     * @brief Return the internal CUDD manager (for advanced users)
     * @return Pointer to the CUDD manager
     */
    DdManager* getManager() const { return manager; }
    
private:
    DdManager* manager;
};

} // namespace Solvers

#endif // BDD_H 