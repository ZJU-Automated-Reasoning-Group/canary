#include"clusterSolver.h"
#include<mutex>
#include<queue>

using namespace std;

clusterSolver::clusterSolver(int max_threads)
	: max_threads(max_threads), queue_lock(), available_solver() {
	for (int i = 0; i < max_threads; ++i) { available_solver.push(baseSolver()); }
}

checkResults clusterSolver::incre_solve(int assertionId, std::vector<generalExpr> eval_target) {
	printf("called incre_solve\n");
	while (true) {
		queue_lock.lock();
		if (available_solver.size()) {
			baseSolver& cur = available_solver.front();
			available_solver.pop();
			queue_lock.unlock();
			cur.sync_to_id(assertionId);
			return cur.check(eval_target);
		}
		queue_lock.unlock();
	}
}