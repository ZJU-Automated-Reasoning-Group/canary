/**
 * @file ModelCheckingExample.cpp
 * @brief Example demonstrating BDD-based symbolic model checking
 * 
 * This example implements symbolic model checking for a simple finite state 
 * system representing a mutual exclusion protocol with two processes.
 */

#include "Solvers/BDD.h"
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

using namespace Solvers;

// Class representing a finite state transition system for model checking
class TransitionSystem {
private:
    BDD& bdd;
    int numStateVars;
    
    // Variables for current state
    std::vector<BDD::Node> stateVars;
    
    // Variables for next state
    std::vector<BDD::Node> nextStateVars;
    
    // The transition relation: R(s, s')
    BDD::Node transitionRelation;
    
    // Initial state predicate: I(s)
    BDD::Node initialState;
    
public:
    TransitionSystem(BDD& bddManager, int numVars) 
        : bdd(bddManager), numStateVars(numVars), 
          stateVars(numVars), nextStateVars(numVars) {
        
        // Initialize state variables
        for (int i = 0; i < numStateVars; i++) {
            stateVars[i] = bdd.makeVar(i);
            // Next state variables start after current state variables
            nextStateVars[i] = bdd.makeVar(i + numStateVars);
        }
        
        transitionRelation = bdd.makeOne(); // Default to true
        initialState = bdd.makeOne();       // Default to true
    }
    
    // Get the transition relation
    BDD::Node getTransitionRelation() const {
        return transitionRelation;
    }
    
    // Get the initial state predicate
    BDD::Node getInitialState() const {
        return initialState;
    }
    
    // Set the transition relation
    void setTransitionRelation(const BDD::Node& relation) {
        transitionRelation = relation;
    }
    
    // Set the initial state predicate
    void setInitialState(const BDD::Node& state) {
        initialState = state;
    }
    
    // Get a state variable
    BDD::Node getStateVar(int index) const {
        if (index < 0 || index >= numStateVars) {
            throw std::out_of_range("State variable index out of range");
        }
        return stateVars[index];
    }
    
    // Get a next state variable
    BDD::Node getNextStateVar(int index) const {
        if (index < 0 || index >= numStateVars) {
            throw std::out_of_range("Next state variable index out of range");
        }
        return nextStateVars[index];
    }
    
    // Create a variable equality predicate (v_i = v_i')
    BDD::Node createVarEquality(int index) const {
        if (index < 0 || index >= numStateVars) {
            throw std::out_of_range("Variable index out of range");
        }
        return stateVars[index].iff(nextStateVars[index]);
    }
    
    // Create frame condition: variables not in the provided list remain unchanged
    BDD::Node createFrameCondition(const std::vector<int>& changedVars) const {
        BDD::Node result = bdd.makeOne();
        
        std::vector<bool> isChanged(numStateVars, false);
        for (int var : changedVars) {
            if (var >= 0 && var < numStateVars) {
                isChanged[var] = true;
            }
        }
        
        for (int i = 0; i < numStateVars; i++) {
            if (!isChanged[i]) {
                result = result & createVarEquality(i);
            }
        }
        
        return result;
    }
    
    // Perform substitution of next-state variables with current-state variables
    BDD::Node substituteNextWithCurrent(const BDD::Node& formula) const {
        BDD::Node result = formula;
        for (int i = 0; i < numStateVars; i++) {
            result = result.substitute(i + numStateVars, stateVars[i]);
        }
        return result;
    }
    
    // Compute the set of states reachable from the initial states in one step
    BDD::Node computePost(const BDD::Node& states) const {
        // Compute states & R(s, s')
        BDD::Node conj = states & transitionRelation;
        
        // Existentially quantify current state variables
        BDD::Node result = conj;
        for (int i = 0; i < numStateVars; i++) {
            result = result.existsQuantify(stateVars[i]);
        }
        
        // Substitute next-state variables with current-state variables
        return substituteNextWithCurrent(result);
    }
    
    // Compute the set of reachable states using fixed-point iteration
    BDD::Node computeReachableStates() const {
        BDD::Node reachable = initialState;
        BDD::Node frontier = initialState;
        
        std::cout << "Computing reachable states..." << std::endl;
        int iteration = 0;
        
        while (true) {
            std::cout << "  Iteration " << ++iteration << ", BDD size: " 
                      << reachable.getNodeCount() << std::endl;
            
            BDD::Node newStates = computePost(frontier) & !reachable;
            
            if (newStates.isZero()) {
                // Fixed point reached
                break;
            }
            
            frontier = newStates;
            reachable = reachable | newStates;
        }
        
        std::cout << "Fixed point reached after " << iteration << " iterations." << std::endl;
        return reachable;
    }
    
    // Check if a safety property holds in all reachable states
    bool checkSafety(const BDD::Node& property) const {
        BDD::Node reachable = computeReachableStates();
        BDD::Node violation = reachable & !property;
        
        return violation.isZero();
    }
    
    // Compute the set of states satisfying EF(target) - states that can reach target
    BDD::Node computeEF(const BDD::Node& target) const {
        BDD::Node result = target;
        BDD::Node prevResult;
        
        std::cout << "Computing EF..." << std::endl;
        int iteration = 0;
        
        do {
            std::cout << "  Iteration " << ++iteration << ", BDD size: " 
                      << result.getNodeCount() << std::endl;
            
            prevResult = result;
            
            // Add states that can reach the current result in one step
            BDD::Node preImage = computePreImage(result);
            result = result | preImage;
            
        } while (!(result.iff(prevResult)).isTautology());
        
        std::cout << "Fixed point reached after " << iteration << " iterations." << std::endl;
        return result;
    }
    
    // Compute the pre-image of a set of states: states that can reach the target in one step
    BDD::Node computePreImage(const BDD::Node& target) const {
        // Substitute current state variables with next state variables in the target
        BDD::Node nextTarget = target;
        for (int i = 0; i < numStateVars; i++) {
            nextTarget = nextTarget.substitute(i, nextStateVars[i]);
        }
        
        // Conjunction with transition relation
        BDD::Node conj = transitionRelation & nextTarget;
        
        // Existentially quantify next state variables
        BDD::Node result = conj;
        for (int i = 0; i < numStateVars; i++) {
            result = result.existsQuantify(nextStateVars[i]);
        }
        
        return result;
    }
    
    // Get the state variable count
    int getStateVarCount() const {
        return numStateVars;
    }
};

// Function to define a basic mutual exclusion protocol
TransitionSystem defineMutualExclusionProtocol(BDD& bdd) {
    // Define state variables:
    // 0: process 1 in critical section
    // 1: process 1 is trying to enter
    // 2: process 2 in critical section
    // 3: process 2 is trying to enter
    TransitionSystem ts(bdd, 4);
    
    // Convenience aliases for state variables
    BDD::Node p1_in_cs = ts.getStateVar(0);
    BDD::Node p1_trying = ts.getStateVar(1);
    BDD::Node p2_in_cs = ts.getStateVar(2);
    BDD::Node p2_trying = ts.getStateVar(3);
    
    BDD::Node p1_in_cs_next = ts.getNextStateVar(0);
    BDD::Node p1_trying_next = ts.getNextStateVar(1);
    BDD::Node p2_in_cs_next = ts.getNextStateVar(2);
    BDD::Node p2_trying_next = ts.getNextStateVar(3);
    
    // Initial state: No process is in critical section or trying
    BDD::Node initialState = !p1_in_cs & !p1_trying & !p2_in_cs & !p2_trying;
    ts.setInitialState(initialState);
    
    // Define transitions
    
    // 1. Process 1 starts trying to enter critical section
    BDD::Node t1 = !p1_trying & !p1_in_cs & !p1_trying_next & p1_trying_next;
    t1 = t1 & ts.createFrameCondition({1}); // Only p1_trying changes
    
    // 2. Process 1 enters critical section if trying and process 2 is not in CS
    BDD::Node t2 = p1_trying & !p1_in_cs & !p2_in_cs & p1_in_cs_next & !p1_trying_next;
    t2 = t2 & ts.createFrameCondition({0, 1}); // Only p1_in_cs and p1_trying change
    
    // 3. Process 1 exits critical section
    BDD::Node t3 = p1_in_cs & !p1_in_cs_next & !p1_trying_next;
    t3 = t3 & ts.createFrameCondition({0, 1}); // Only p1_in_cs and p1_trying change
    
    // 4. Process 2 starts trying to enter critical section
    BDD::Node t4 = !p2_trying & !p2_in_cs & p2_trying_next & !p2_in_cs_next;
    t4 = t4 & ts.createFrameCondition({3}); // Only p2_trying changes
    
    // 5. Process 2 enters critical section if trying and process 1 is not in CS
    BDD::Node t5 = p2_trying & !p2_in_cs & !p1_in_cs & p2_in_cs_next & !p2_trying_next;
    t5 = t5 & ts.createFrameCondition({2, 3}); // Only p2_in_cs and p2_trying change
    
    // 6. Process 2 exits critical section
    BDD::Node t6 = p2_in_cs & !p2_in_cs_next & !p2_trying_next;
    t6 = t6 & ts.createFrameCondition({2, 3}); // Only p2_in_cs and p2_trying change
    
    // Combine all transitions
    BDD::Node transitionRelation = t1 | t2 | t3 | t4 | t5 | t6;
    ts.setTransitionRelation(transitionRelation);
    
    return ts;
}

int main() {
    // Create a BDD manager with enough variables for states and next states
    // We need 8 variables: 4 for current state and 4 for next state
    BDD bdd(8);
    
    std::cout << "BDD-Based Symbolic Model Checking Example" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    // Define the mutual exclusion protocol
    TransitionSystem ts = defineMutualExclusionProtocol(bdd);
    
    // Verify mutual exclusion: The two processes can't be in the critical section simultaneously
    BDD::Node p1_in_cs = ts.getStateVar(0);
    BDD::Node p2_in_cs = ts.getStateVar(2);
    BDD::Node mutualExclusion = !(p1_in_cs & p2_in_cs);
    
    std::cout << "\nChecking mutual exclusion safety property..." << std::endl;
    bool isSafe = ts.checkSafety(mutualExclusion);
    
    if (isSafe) {
        std::cout << "Mutual exclusion property HOLDS: The two processes can't be in the critical section simultaneously." << std::endl;
    } else {
        std::cout << "Mutual exclusion property VIOLATED: The two processes can be in the critical section simultaneously." << std::endl;
    }
    
    // Verify absence of deadlock: There's always a way for a process to make progress if it's trying
    // A process can make progress if it can enter the critical section at some point in the future
    std::cout << "\nChecking absence of deadlock..." << std::endl;
    
    BDD::Node p1_trying = ts.getStateVar(1);
    BDD::Node p2_trying = ts.getStateVar(3);
    
    // If p1 is trying, it should be able to reach critical section
    BDD::Node p1CanEnter = ts.computeEF(p1_in_cs);
    BDD::Node p1Progress = !p1_trying | p1CanEnter;
    
    // If p2 is trying, it should be able to reach critical section
    BDD::Node p2CanEnter = ts.computeEF(p2_in_cs);
    BDD::Node p2Progress = !p2_trying | p2CanEnter;
    
    // Deadlock freedom: all reachable states allow progress
    BDD::Node deadlockFreedom = p1Progress & p2Progress;
    bool isDeadlockFree = ts.checkSafety(deadlockFreedom);
    
    if (isDeadlockFree) {
        std::cout << "Deadlock freedom property HOLDS: Processes can always make progress." << std::endl;
    } else {
        std::cout << "Deadlock freedom property VIOLATED: Deadlocks are possible." << std::endl;
    }
    
    // Generate DOT graphs for visualization
    BDD::Node reachableStates = ts.computeReachableStates();
    std::cout << "\nGenerating visualization of reachable states..." << std::endl;
    reachableStates.printDot("reachable_states.dot", {"p1_in_cs", "p1_trying", "p2_in_cs", "p2_trying"});
    std::cout << "Created reachable_states.dot" << std::endl;
    
    std::cout << "\nGenerating visualization of transition relation..." << std::endl;
    ts.getTransitionRelation().printDot("transition_relation.dot", 
            {"p1_in_cs", "p1_trying", "p2_in_cs", "p2_trying", 
             "p1_in_cs'", "p1_trying'", "p2_in_cs'", "p2_trying'"});
    std::cout << "Created transition_relation.dot" << std::endl;
    
    return 0;
} 