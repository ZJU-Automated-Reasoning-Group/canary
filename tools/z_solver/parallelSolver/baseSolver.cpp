#include<stack>
#include"baseSolver.h"
#include"modelSolver/assertionDatabase.h"

using namespace std;

extern assertionDatabase _BASE_ENV;

checkResults::checkResults(z3::check_result res, vector<generalExpr> eval)
	: res(res), eval(eval) {
}

baseSolver::baseSolver()
    : ctx(make_unique<z3::context>()), solver(*ctx), id(0) {
}

void baseSolver::sync_to_id(int assertionId) {
	printf("called sync_to_id\n");
	int lca = assertionDatabase::getLCA(id, assertionId);
	stack<generalExpr> id_s;
	while (assertionId != lca) {
		id_s.push(_BASE_ENV.node_expr[assertionId]);
		assertionId = _BASE_ENV.father[0][assertionId];
	}
	while (id != lca) {
		id = _BASE_ENV.father[0][id];
		solver.pop();
	}
	while (id_s.size()) {
		solver.add(id_s.top().expr_for_ctx(solver.ctx()));
		id_s.pop();
		solver.push();
	}
}

checkResults baseSolver::check(std::vector<generalExpr> eval_target) {
	vector<generalExpr> eval;
	z3::check_result res = solver.check();
	if (res == z3::sat) {
		z3::model model = solver.get_model();
		for (int i = 0; i < eval_target.size(); ++i) { eval.push_back(model.eval(eval_target[i].expr_for_ctx(model.ctx()), true)); }
	}
	return checkResults(res, eval);
}