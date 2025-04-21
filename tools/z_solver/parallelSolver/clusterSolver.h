#pragma once
#include<mutex>
#include<queue>
#include"baseSolver.h"
#include"modelSolver/generalExpr.h"

struct clusterSolver {
	int max_threads;
	std::mutex queue_lock;
	std::queue<baseSolver> available_solver;

	clusterSolver(int max_threads);
	checkResults incre_solve(int assertionId, std::vector<generalExpr> eval_target);
};