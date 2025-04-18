/**
 * @file BDD.cpp
 * @brief Implementation of the user-friendly BDD wrapper for CUDD
 */

#include "Solvers/BDD.h"
#include <algorithm>
#include <cmath>
#include <cstdio>  // For FILE operations
#include <cstring>  // For strcpy
#include <fstream>
#include <iostream>
#include <queue>
#include <cstdint>
#include <unordered_set>

namespace Solvers {

// BDD main class implementation

BDD::BDD(unsigned int numVars, 
         unsigned int numVarsZ, 
         unsigned int numSlots, 
         unsigned int cacheSize, 
         unsigned long maxMemory) {
    manager = Cudd_Init(numVars, numVarsZ, numSlots, cacheSize, maxMemory);
    if (!manager) {
        throw std::runtime_error("Failed to initialize CUDD manager");
    }
}

BDD::~BDD() {
    if (manager) {
        Cudd_Quit(manager);
        manager = nullptr;
    }
}

BDD::BDD(BDD&& other) noexcept : manager(other.manager) {
    other.manager = nullptr;
}

BDD& BDD::operator=(BDD&& other) noexcept {
    if (this != &other) {
        if (manager) {
            Cudd_Quit(manager);
        }
        manager = other.manager;
        other.manager = nullptr;
    }
    return *this;
}

BDD::Node BDD::makeVar(int index) {
    if (index < 0) {
        throw std::invalid_argument("Variable index must be non-negative");
    }
    
    DdNode* node = Cudd_bddIthVar(manager, index);
    if (!node) {
        throw std::runtime_error("Failed to create variable node");
    }
    
    Cudd_Ref(node);
    return Node(manager, node);
}

BDD::Node BDD::makeNotVar(int index) {
    if (index < 0) {
        throw std::invalid_argument("Variable index must be non-negative");
    }
    
    DdNode* var = Cudd_bddIthVar(manager, index);
    if (!var) {
        throw std::runtime_error("Failed to create variable node");
    }
    
    DdNode* node = Cudd_Not(var);
    Cudd_Ref(node);
    return Node(manager, node);
}

BDD::Node BDD::makeOne() {
    DdNode* node = Cudd_ReadOne(manager);
    Cudd_Ref(node);
    return Node(manager, node);
}

BDD::Node BDD::makeZero() {
    DdNode* node = Cudd_ReadLogicZero(manager);
    Cudd_Ref(node);
    return Node(manager, node);
}

BDD::Node BDD::makeCube(const std::vector<int>& vars, const std::vector<bool>& phase) {
    if (vars.size() != phase.size()) {
        throw std::invalid_argument("Vars and phase vectors must have the same size");
    }
    
    int n = vars.size();
    std::vector<int> phaseInt(n);
    for (int i = 0; i < n; i++) {
        phaseInt[i] = phase[i] ? 1 : 0;
    }
    
    std::vector<DdNode*> varNodes(n);
    for (int i = 0; i < n; i++) {
        varNodes[i] = Cudd_bddIthVar(manager, vars[i]);
    }
    
    DdNode* cube = Cudd_bddComputeCube(manager, varNodes.data(), phaseInt.data(), n);
    if (!cube) {
        throw std::runtime_error("Failed to create cube");
    }
    
    Cudd_Ref(cube);
    return Node(manager, cube);
}

BDD::Node BDD::makeFromFunction(const std::function<bool(const std::vector<bool>&)>& func, int numVars) {
    if (numVars <= 0) {
        throw std::invalid_argument("Number of variables must be positive");
    }
    
    // Start with the constant representing false
    Node result = makeZero();
    
    // Iterate through all possible input combinations
    int numCombinations = 1 << numVars;
    for (int i = 0; i < numCombinations; i++) {
        // Create input vector
        std::vector<bool> input(numVars);
        for (int j = 0; j < numVars; j++) {
            input[j] = (i & (1 << j)) != 0;
        }
        
        // Evaluate function
        if (func(input)) {
            // Create a cube for this input combination
            std::vector<int> vars(numVars);
            std::vector<bool> phase(numVars);
            for (int j = 0; j < numVars; j++) {
                vars[j] = j;
                phase[j] = input[j];
            }
            
            Node cube = makeCube(vars, phase);
            Node oldResult = result;
            result = oldResult | cube;
        }
    }
    
    return result;
}

BDD::Node BDD::makeFromTruthTable(const std::vector<bool>& truthTable, int numVars) {
    if (truthTable.size() != (1u << numVars)) {
        throw std::invalid_argument("Truth table size must be 2^numVars");
    }
    
    auto func = [&truthTable, numVars](const std::vector<bool>& input) -> bool {
        int idx = 0;
        for (int i = 0; i < numVars; i++) {
            if (input[i]) {
                idx |= (1 << i);
            }
        }
        return truthTable[idx];
    };
    
    return makeFromFunction(func, numVars);
}

BDD::Node BDD::makeEquality(const std::vector<int>& aVars, const std::vector<int>& bVars) {
    if (aVars.size() != bVars.size()) {
        throw std::invalid_argument("Variable vectors must have the same size");
    }
    
    Node result = makeOne();
    for (size_t i = 0; i < aVars.size(); i++) {
        Node a = makeVar(aVars[i]);
        Node b = makeVar(bVars[i]);
        Node equal = a.iff(b);
        result = result & equal;
    }
    
    return result;
}

BDD::Node BDD::makeLessThan(const std::vector<int>& aVars, const std::vector<int>& bVars) {
    if (aVars.size() != bVars.size()) {
        throw std::invalid_argument("Variable vectors must have the same size");
    }
    
    Node result = makeZero();
    Node sofar = makeOne();
    
    // For a < b, we need: a[i-1:0] == b[i-1:0] && !a[i] && b[i]
    // where i is the most significant bit where a and b differ
    for (int i = aVars.size() - 1; i >= 0; i--) {
        Node a = makeVar(aVars[i]);
        Node b = makeVar(bVars[i]);
        
        Node partial = sofar & !a & b;
        result = result | partial;
        
        Node equal = a.iff(b);
        sofar = sofar & equal;
    }
    
    return result;
}

void BDD::enableReordering(Cudd_ReorderingType method) {
    Cudd_AutodynEnable(manager, method);
}

void BDD::disableReordering() {
    Cudd_AutodynDisable(manager);
}

bool BDD::reorderVariables(Cudd_ReorderingType method) {
    return Cudd_ReduceHeap(manager, method, 0) == 1;
}

// BDD::Node implementation

BDD::Node::Node(DdManager* mgr, DdNode* n) : manager(mgr), node(n) {
    // Reference already added in factory methods
}

BDD::Node::~Node() {
    if (manager && node) {
        Cudd_RecursiveDeref(manager, node);
    }
}

BDD::Node::Node(const Node& other) : manager(other.manager), node(other.node) {
    if (manager && node) {
        Cudd_Ref(node);
    }
}

BDD::Node::Node(Node&& other) noexcept : manager(other.manager), node(other.node) {
    other.manager = nullptr;
    other.node = nullptr;
}

BDD::Node& BDD::Node::operator=(const Node& other) {
    if (this != &other) {
        if (manager && node) {
            Cudd_RecursiveDeref(manager, node);
        }
        
        manager = other.manager;
        node = other.node;
        
        if (manager && node) {
            Cudd_Ref(node);
        }
    }
    return *this;
}

BDD::Node& BDD::Node::operator=(Node&& other) noexcept {
    if (this != &other) {
        if (manager && node) {
            Cudd_RecursiveDeref(manager, node);
        }
        
        manager = other.manager;
        node = other.node;
        
        other.manager = nullptr;
        other.node = nullptr;
    }
    return *this;
}

BDD::Node BDD::Node::operator&(const Node& other) const {
    if (!manager || !node || !other.manager || !other.node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (manager != other.manager) {
        throw std::invalid_argument("BDD nodes must belong to the same manager");
    }
    
    DdNode* result = Cudd_bddAnd(manager, node, other.node);
    if (!result) {
        throw std::runtime_error("BDD operation failed");
    }
    
    Cudd_Ref(result);
    return Node(manager, result);
}

BDD::Node BDD::Node::operator|(const Node& other) const {
    if (!manager || !node || !other.manager || !other.node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (manager != other.manager) {
        throw std::invalid_argument("BDD nodes must belong to the same manager");
    }
    
    DdNode* result = Cudd_bddOr(manager, node, other.node);
    if (!result) {
        throw std::runtime_error("BDD operation failed");
    }
    
    Cudd_Ref(result);
    return Node(manager, result);
}

BDD::Node BDD::Node::operator^(const Node& other) const {
    if (!manager || !node || !other.manager || !other.node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (manager != other.manager) {
        throw std::invalid_argument("BDD nodes must belong to the same manager");
    }
    
    DdNode* result = Cudd_bddXor(manager, node, other.node);
    if (!result) {
        throw std::runtime_error("BDD operation failed");
    }
    
    Cudd_Ref(result);
    return Node(manager, result);
}

BDD::Node BDD::Node::operator!() const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    DdNode* result = Cudd_Not(node);
    Cudd_Ref(result);
    return Node(manager, result);
}

BDD::Node BDD::Node::implies(const Node& other) const {
    if (!manager || !node || !other.manager || !other.node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (manager != other.manager) {
        throw std::invalid_argument("BDD nodes must belong to the same manager");
    }
    
    // a => b is equivalent to !a | b
    DdNode* notNode = Cudd_Not(node);
    DdNode* result = Cudd_bddOr(manager, notNode, other.node);
    if (!result) {
        throw std::runtime_error("BDD operation failed");
    }
    
    Cudd_Ref(result);
    return Node(manager, result);
}

BDD::Node BDD::Node::iff(const Node& other) const {
    if (!manager || !node || !other.manager || !other.node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (manager != other.manager) {
        throw std::invalid_argument("BDD nodes must belong to the same manager");
    }
    
    DdNode* result = Cudd_bddXnor(manager, node, other.node);
    if (!result) {
        throw std::runtime_error("BDD operation failed");
    }
    
    Cudd_Ref(result);
    return Node(manager, result);
}

BDD::Node BDD::Node::existsQuantify(const Node& vars) const {
    if (!manager || !node || !vars.manager || !vars.node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (manager != vars.manager) {
        throw std::invalid_argument("BDD nodes must belong to the same manager");
    }
    
    DdNode* result = Cudd_bddExistAbstract(manager, node, vars.node);
    if (!result) {
        throw std::runtime_error("BDD operation failed");
    }
    
    Cudd_Ref(result);
    return Node(manager, result);
}

BDD::Node BDD::Node::universalQuantify(const Node& vars) const {
    if (!manager || !node || !vars.manager || !vars.node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (manager != vars.manager) {
        throw std::invalid_argument("BDD nodes must belong to the same manager");
    }
    
    DdNode* result = Cudd_bddUnivAbstract(manager, node, vars.node);
    if (!result) {
        throw std::runtime_error("BDD operation failed");
    }
    
    Cudd_Ref(result);
    return Node(manager, result);
}

BDD::Node BDD::Node::substitute(int varIndex, const Node& replacement) const {
    if (!manager || !node || !replacement.manager || !replacement.node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (manager != replacement.manager) {
        throw std::invalid_argument("BDD nodes must belong to the same manager");
    }
    
    // Instead of Cudd_bddSwapVariables, implement manually using BDD operations
    // f[x/g] = (x & f_x) | (!x & f_!x)
    // where f_x is the positive cofactor of f with respect to x,
    // and f_!x is the negative cofactor of f with respect to x.
    
    DdNode* var = Cudd_bddIthVar(manager, varIndex);
    if (!var) {
        throw std::invalid_argument("Invalid variable index");
    }
    
    // Compute positive and negative cofactors
    DdNode* f_pos = Cudd_Cofactor(manager, node, var);
    if (!f_pos) {
        throw std::runtime_error("BDD operation failed");
    }
    Cudd_Ref(f_pos);
    
    DdNode* neg_var = Cudd_Not(var);
    DdNode* f_neg = Cudd_Cofactor(manager, node, neg_var);
    if (!f_neg) {
        Cudd_RecursiveDeref(manager, f_pos);
        throw std::runtime_error("BDD operation failed");
    }
    Cudd_Ref(f_neg);
    
    // Compute (replacement & f_pos)
    DdNode* term1 = Cudd_bddAnd(manager, replacement.node, f_pos);
    if (!term1) {
        Cudd_RecursiveDeref(manager, f_pos);
        Cudd_RecursiveDeref(manager, f_neg);
        throw std::runtime_error("BDD operation failed");
    }
    Cudd_Ref(term1);
    
    // Compute (!replacement & f_neg)
    DdNode* not_replacement = Cudd_Not(replacement.node);
    DdNode* term2 = Cudd_bddAnd(manager, not_replacement, f_neg);
    if (!term2) {
        Cudd_RecursiveDeref(manager, f_pos);
        Cudd_RecursiveDeref(manager, f_neg);
        Cudd_RecursiveDeref(manager, term1);
        throw std::runtime_error("BDD operation failed");
    }
    Cudd_Ref(term2);
    
    // Compute the result: term1 | term2
    DdNode* result = Cudd_bddOr(manager, term1, term2);
    if (!result) {
        Cudd_RecursiveDeref(manager, f_pos);
        Cudd_RecursiveDeref(manager, f_neg);
        Cudd_RecursiveDeref(manager, term1);
        Cudd_RecursiveDeref(manager, term2);
        throw std::runtime_error("BDD operation failed");
    }
    Cudd_Ref(result);
    
    // Clean up
    Cudd_RecursiveDeref(manager, f_pos);
    Cudd_RecursiveDeref(manager, f_neg);
    Cudd_RecursiveDeref(manager, term1);
    Cudd_RecursiveDeref(manager, term2);
    
    return Node(manager, result);
}

bool BDD::Node::isTautology() const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    DdNode* one = Cudd_ReadOne(manager);
    return node == one;
}

bool BDD::Node::isSatisfiable() const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    DdNode* zero = Cudd_ReadLogicZero(manager);
    return node != zero;
}

bool BDD::Node::isZero() const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    DdNode* zero = Cudd_ReadLogicZero(manager);
    return node == zero;
}

bool BDD::Node::isOne() const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    DdNode* one = Cudd_ReadOne(manager);
    return node == one;
}

int BDD::Node::getIndex() const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (Cudd_IsConstant(node)) {
        throw std::runtime_error("Constant nodes do not have an index");
    }
    
    return Cudd_NodeReadIndex(node);
}

BDD::Node BDD::Node::getThen() const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (Cudd_IsConstant(node)) {
        throw std::runtime_error("Constant nodes do not have Then/Else branches");
    }
    
    DdNode* regular = Cudd_Regular(node);
    DdNode* thenBranch = Cudd_T(regular);
    
    // If the node is complemented, negate the result
    if (Cudd_IsComplement(node)) {
        thenBranch = Cudd_Not(thenBranch);
    }
    
    Cudd_Ref(thenBranch);
    return Node(manager, thenBranch);
}

BDD::Node BDD::Node::getElse() const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (Cudd_IsConstant(node)) {
        throw std::runtime_error("Constant nodes do not have Then/Else branches");
    }
    
    DdNode* regular = Cudd_Regular(node);
    DdNode* elseBranch = Cudd_E(regular);
    
    // If the node is complemented, negate the result
    if (Cudd_IsComplement(node)) {
        elseBranch = Cudd_Not(elseBranch);
    }
    
    Cudd_Ref(elseBranch);
    return Node(manager, elseBranch);
}

std::vector<bool> BDD::Node::getSatisfyingAssignment() const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (isZero()) {
        throw std::runtime_error("No satisfying assignment exists");
    }
    
    int size = Cudd_ReadSize(manager);
    std::vector<bool> result(size, false);
    
    // Create an array of variables
    std::vector<DdNode*> vars(size);
    for (int i = 0; i < size; i++) {
        vars[i] = Cudd_bddIthVar(manager, i);
    }
    
    // Get a satisfying assignment
    DdNode* minterm = Cudd_bddPickOneMinterm(manager, node, vars.data(), size);
    if (!minterm) {
        throw std::runtime_error("Failed to find a satisfying assignment");
    }
    
    // Extract the assignment
    for (int i = 0; i < size; i++) {
        DdNode* var = vars[i];
        DdNode* tmp = Cudd_bddAnd(manager, minterm, var);
        Cudd_Ref(tmp);
        
        result[i] = !Cudd_IsComplement(tmp);
        
        Cudd_RecursiveDeref(manager, tmp);
    }
    
    Cudd_RecursiveDeref(manager, minterm);
    return result;
}

std::vector<std::vector<bool>> BDD::Node::getAllSatisfyingAssignments() const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (isZero()) {
        return {};
    }
    
    int size = Cudd_ReadSize(manager);
    std::vector<std::vector<bool>> result;
    
    // Generate all paths to 1 in the BDD
    DdGen* gen;
    int* cube;
    CUDD_VALUE_TYPE value;
    
    gen = Cudd_FirstCube(manager, node, &cube, &value);
    if (!gen) {
        throw std::runtime_error("Failed to generate satisfying assignments");
    }
    
    do {
        std::vector<bool> assignment(size, false);
        for (int i = 0; i < size; i++) {
            if (cube[i] == 0) {
                assignment[i] = false;
            } else if (cube[i] == 1) {
                assignment[i] = true;
            } 
            // If cube[i] == 2, the variable doesn't matter, so we can leave it as false
        }
        result.push_back(assignment);
    } while (Cudd_NextCube(gen, &cube, &value) == 1);
    
    Cudd_GenFree(gen);
    return result;
}

double BDD::Node::countSatisfyingAssignments(int numVars) const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    if (numVars < 0) {
        throw std::invalid_argument("Number of variables must be non-negative");
    }
    
    return Cudd_CountMinterm(manager, node, numVars);
}

std::vector<int> BDD::Node::getSupportVariables() const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    int* support = nullptr;
    int size = Cudd_SupportIndices(manager, node, &support);
    
    if (size < 0) {
        throw std::runtime_error("Failed to get support variables");
    }
    
    std::vector<int> result(support, support + size);
    free(support);
    
    return result;
}

int BDD::Node::getNodeCount() const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    return Cudd_DagSize(node);
}

void BDD::Node::printDot(const std::string& filename, const std::vector<std::string>& varNames) const {
    if (!manager || !node) {
        throw std::runtime_error("Invalid BDD node");
    }
    
    // Since Cudd_DumpDot is not available, provide a simpler visualization
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    // Write a simple DOT file header
    file << "digraph BDD {\n";
    file << "  node [shape=circle];\n";
    
    // Create a map of seen nodes
    std::unordered_set<DdNode*> seen;
    std::queue<DdNode*> queue;
    
    queue.push(node);
    seen.insert(node);
    
    // BFS traversal to output all nodes and edges
    while (!queue.empty()) {
        DdNode* current = queue.front();
        queue.pop();
        
        // Skip if it's a constant (leaf) node
        if (Cudd_IsConstant(current)) {
            int value = Cudd_V(current) == 1.0 ? 1 : 0;
            file << "  \"" << (uintptr_t)current << "\" [label=\"" << value 
                 << "\", shape=box, style=filled, fillcolor=" 
                 << (value ? "green" : "red") << "];\n";
            continue;
        }
        
        // Get index and create label
        int index = Cudd_NodeReadIndex(current);
        std::string label = (index < (int)varNames.size()) ? varNames[index] : "x" + std::to_string(index);
        
        file << "  \"" << (uintptr_t)current << "\" [label=\"" << label << "\"];\n";
        
        // Get then and else branches
        DdNode* tBranch = Cudd_T(current);
        DdNode* eBranch = Cudd_E(current);
        
        // Add edges
        file << "  \"" << (uintptr_t)current << "\" -> \"" << (uintptr_t)tBranch 
             << "\" [style=solid];\n";
        file << "  \"" << (uintptr_t)current << "\" -> \"" << (uintptr_t)eBranch 
             << "\" [style=dashed];\n";
        
        // Add children to queue if not seen before
        if (seen.find(tBranch) == seen.end()) {
            seen.insert(tBranch);
            queue.push(tBranch);
        }
        
        if (seen.find(eBranch) == seen.end()) {
            seen.insert(eBranch);
            queue.push(eBranch);
        }
    }
    
    file << "}\n";
    file.close();
}

} // namespace Solvers 