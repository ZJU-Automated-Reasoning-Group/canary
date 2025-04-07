/**
 * @file BDDExample.cpp
 * @brief Example demonstrating the user-friendly BDD class
 */

#include "Solvers/BDD.h"
#include <iostream>
#include <vector>
#include <string>

using namespace Solvers;

int main() {
    // Create a BDD manager with 10 variables
    BDD bdd(10);
    
    std::cout << "BDD Example - User-friendly BDD wrapper for CUDD" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    // Example 1: Simple operations
    std::cout << "\nExample 1: Simple Boolean operations" << std::endl;
    
    // Create variables x0, x1, x2
    BDD::Node x0 = bdd.makeVar(0);
    BDD::Node x1 = bdd.makeVar(1);
    BDD::Node x2 = bdd.makeVar(2);
    
    // Create the function: f = (x0 & x1) | (!x0 & x2)
    BDD::Node f = (x0 & x1) | (!x0 & x2);
    
    std::cout << "Created function f = (x0 & x1) | (!x0 & x2)" << std::endl;
    std::cout << "Number of nodes in the BDD: " << f.getNodeCount() << std::endl;
    
    // Check if the function is satisfiable
    std::cout << "Is f satisfiable? " << (f.isSatisfiable() ? "Yes" : "No") << std::endl;
    
    // Get one satisfying assignment
    if (f.isSatisfiable()) {
        std::vector<bool> assignment = f.getSatisfyingAssignment();
        std::cout << "One satisfying assignment: x0=" << assignment[0] 
                  << ", x1=" << assignment[1] 
                  << ", x2=" << assignment[2] << std::endl;
    }
    
    // Count the number of satisfying assignments
    std::cout << "Number of satisfying assignments: " << f.countSatisfyingAssignments(3) << std::endl;
    
    // Example 2: Creating functions from truth tables
    std::cout << "\nExample 2: Creating a function from a truth table" << std::endl;
    
    // Truth table for a full adder's sum output (x0 ^ x1 ^ x2)
    // Format: {f(000), f(001), f(010), f(011), f(100), f(101), f(110), f(111)}
    std::vector<bool> truthTable = {false, true, true, false, true, false, false, true};
    
    BDD::Node sum = bdd.makeFromTruthTable(truthTable, 3);
    std::cout << "Created a full adder's sum function from truth table" << std::endl;
    std::cout << "Number of nodes in the BDD: " << sum.getNodeCount() << std::endl;
    
    // Verify: sum should equal x0 ^ x1 ^ x2
    BDD::Node xor_function = x0 ^ x1 ^ x2;
    bool areEqual = (sum.iff(xor_function)).isTautology();
    std::cout << "Sum equals x0 ^ x1 ^ x2: " << (areEqual ? "Yes" : "No") << std::endl;
    
    // Example 3: Variable quantification
    std::cout << "\nExample 3: Variable quantification" << std::endl;
    
    // Create a function g = (x0 & x1 & x3) | (x2 & x3)
    BDD::Node x3 = bdd.makeVar(3);
    BDD::Node g = (x0 & x1 & x3) | (x2 & x3);
    
    std::cout << "Created function g = (x0 & x1 & x3) | (x2 & x3)" << std::endl;
    
    // Existentially quantify x3
    BDD::Node exists_x3 = g.existsQuantify(x3);
    std::cout << "After existentially quantifying x3: " << exists_x3.getNodeCount() << " nodes" << std::endl;
    
    // Universally quantify x0
    BDD::Node forall_x0 = g.universalQuantify(x0);
    std::cout << "After universally quantifying x0: " << forall_x0.getNodeCount() << " nodes" << std::endl;
    
    // Example 4: Generating dot file for visualization
    std::cout << "\nExample 4: Generating visualization" << std::endl;
    
    // Create a function h = (x0 & x1) | (x2 & x3) | (x4 & x5)
    BDD::Node x4 = bdd.makeVar(4);
    BDD::Node x5 = bdd.makeVar(5);
    BDD::Node h = (x0 & x1) | (x2 & x3) | (x4 & x5);
    
    std::cout << "Created function h = (x0 & x1) | (x2 & x3) | (x4 & x5)" << std::endl;
    std::cout << "Number of nodes in the BDD: " << h.getNodeCount() << std::endl;
    
    // Generate a dot file for visualization
    std::vector<std::string> varNames = {"x0", "x1", "x2", "x3", "x4", "x5"};
    h.printDot("h_function.dot", varNames);
    std::cout << "Generated dot file: h_function.dot" << std::endl;
    std::cout << "To visualize, run: dot -Tpng h_function.dot -o h_function.png" << std::endl;
    
    return 0;
} 