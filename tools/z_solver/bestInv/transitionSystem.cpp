// Transition System
#include "transitionSystem.h"

using namespace std;

transitionSystem::transitionSystem(z3::context& ctx, z3::expr pre, z3::expr trans, z3::expr post, z3::expr_vector vars, z3::expr_vector vars_bar)
	: ctx(ctx), pre(pre), trans(trans), post(post), vars(vars), vars_bar(vars_bar) {
}

z3::expr_vector transitionSystem::get_ori_consts() {
	z3::expr_vector ret(ctx);
	for (int i = 0; i < vars.size(); ++i) {
		ret.push_back(vars[i]);
	}
	return ret;
}

z3::expr_vector transitionSystem::get_bar_consts() {
	z3::expr_vector ret(ctx);
	for (int i = 0; i < vars.size(); ++i) {
		ret.push_back(vars_bar[i]);
	}
	return ret;
}

z3::expr_vector transitionSystem::get_all_consts() {
	z3::expr_vector ret(ctx);
	for (int i = 0; i < vars.size(); ++i) {
		ret.push_back(vars[i]);
		ret.push_back(vars_bar[i]);
	}
	return ret;
}