#pragma once
#include"z3++.h"
#include"modelSolver/generalExpr.h"

struct checkResults {
    z3::check_result res;
    std::vector<generalExpr> eval;

    checkResults(z3::check_result res, std::vector<generalExpr> eval);
};

struct baseSolver {
    std::unique_ptr<z3::context> ctx;
    z3::solver solver;
    int id;
    
    baseSolver();

    // Delete copy constructor and assignment
    baseSolver(const baseSolver&) = delete;
    baseSolver& operator=(const baseSolver&) = delete;
    // Allow move operations
    baseSolver(baseSolver&&) = default;
    baseSolver& operator=(baseSolver&&) = default;

    void sync_to_id(int assertionId);
    checkResults check(std::vector<generalExpr> eval_target);
};