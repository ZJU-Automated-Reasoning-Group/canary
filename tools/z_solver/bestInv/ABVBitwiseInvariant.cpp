#include"invTemplate.h"

using namespace std;

ABVBitwiseInvariant::ABVBitwiseInvariant(transitionSystem trans)
	: invariant(trans) {
	size = 0;
	for (unsigned i = 0; i < trans.vars.size(); ++i) {
		int size_cur = trans.vars[i].get_sort().bv_size();
		size += size_cur;
		z3::expr indi = ctx.bv_const((trans.vars[i].to_string() + "_z").c_str(), size_cur);
		vari.push_back(indi);
		for (int j = size_cur - 1; j >= 0; --j) {
			belong.push_back(i);
			pstion.push_back(j);
			varz.push_back(ctx.bool_const((trans.vars[i].to_string() + "_b" + to_string(j)).c_str()));
			inv_cons = inv_cons && ((z3::lshr(trans.vars[i], j) & 1) == indi || varz.back() == ctx.bool_val(false));
			inv_bar_cons = inv_bar_cons && ((z3::lshr(trans.vars_bar[i], j) & 1) == indi || varz.back() == ctx.bool_val(false));
		}
		inv_cons = inv_cons.simplify();
		inv_bar_cons = inv_bar_cons.simplify();
		pre_cons = z3::forall(trans.get_ori_consts(), z3::implies(trans.pre, inv_cons));
		trans_cons = z3::forall(trans.get_all_consts(), z3::implies(inv_cons && trans.trans, inv_bar_cons));
		post_cons = z3::forall(trans.get_ori_consts(), z3::implies(inv_cons, trans.post));
	}
}

string ABVBitwiseInvariant::val_string() {
	if (status.size() == 0) { return ""; }
	string str;
	for (int i = 0; i < size; ++i) {
		if (i && belong[i] != belong[i - 1]) { str += ' '; }
		str += status[i] == -1 ? 'x' : status[i] + '0';
	}
	return str;
}

z3::expr ABVBitwiseInvariant::get_current_inv() {
	z3::expr ret = ctx.bool_val(true);
	for (int i = 0; i < size; ++i) {
		if (status[i] != -1) { ret = ret && (z3::lshr(trans.vars[belong[i]], pstion[i]) & 1) == status[i]; }
	}
	return ret.simplify();
}

z3::expr ABVBitwiseInvariant::get_current_inv_bar() {
	z3::expr ret = ctx.bool_val(true);
	for (int i = 0; i < size; ++i) {
		if (status[i] != -1) { ret = ret && (z3::lshr(trans.vars_bar[belong[i]], pstion[i]) & 1) == status[i]; }
	}
	return ret.simplify();
}

bool ABVBitwiseInvariant::bilateral_abstraction(z3::solver& sol) {
	z3::check_result res = sol.check();
	if (timeout_check()) { return false; }
	if (res != z3::sat) {
		bad_solve = 1;
		return false;
	}
	z3::model model = sol.get_model();
	for (int i = 0; i < size; ++i) { status.push_back(((z3::lshr(model.eval(trans.vars[belong[i]], true), pstion[i]) & 1) == 1).simplify().is_true()); }
	while (true) {
		z3::expr incr = ctx.bool_val(false);
		for (int i = 0; i < size; ++i) {
			if (status[i] != -1) { incr = incr || (z3::lshr(trans.vars[belong[i]], pstion[i]) & 1) != status[i]; }
		}
		sol.push();
		sol.add(incr);
		res = sol.check();
		if (timeout_check()) { return false; }
		++smt_count;
		if (res == z3::sat) {
			model = sol.get_model();
			for (int i = 0; i < size; ++i) {
				if (((z3::lshr(model.eval(trans.vars[belong[i]], true), pstion[i]) & 1) != status[i]).simplify().is_true()) { status[i] = -1; }
			}
			sol.pop();
		}
		else if (res == z3::unsat) {
			sol.pop();
			return true;
		}
		else {
			bad_solve = 1;
			return false;
		}
	}
}

bool ABVBitwiseInvariant::bilateral_abstraction_bar(z3::solver& sol) {
	bool tag = false;
	while (true) {
		z3::expr incr = ctx.bool_val(false);
		for (int i = 0; i < size; ++i) {
			if (status[i] != -1) { incr = incr || (z3::lshr(trans.vars_bar[belong[i]], pstion[i]) & 1) != status[i]; }
		}
		sol.push();
		sol.add(incr);
		z3::check_result res = sol.check();
		if (timeout_check()) { return false; }
		++smt_count;
		if (res == z3::sat) {
			z3::model model = sol.get_model();
			for (int i = 0; i < size; ++i) {
				if (((z3::lshr(model.eval(trans.vars_bar[belong[i]], true), pstion[i]) & 1) != status[i]).simplify().is_true()) { status[i] = -1; }
			}
			sol.pop();
			tag = true;
		}
		else if (res == z3::unsat) {
			sol.pop();
			return tag;
		}
		else {
			bad_solve = 1;
			return false;
		}
	}
}

void ABVBitwiseInvariant::evaluate_to_best_Guess(double timeout) {
	set_timer(timeout);
	for (int i = 0; i < size; ++i) { status.push_back(-1); }
	z3::solver sol(ctx);
	sol.add(pre_cons && trans_cons);
	for (int i = 0; i < size; ++i) {
		if (status[i] == -1) {
			sol.push();
			sol.add(varz[i] == ctx.bool_val(true));
			z3::check_result res = sol.check();
			if (timeout_check()) { return; }
			++smt_count;
			if (res == z3::sat) {
				z3::model model = sol.get_model();
				for (int j = i; j < size; ++j) {
					if ((model.eval(varz[j], true) == ctx.bool_val(true)).simplify().is_true()) { status[j] = (model.eval(vari[belong[j]], true) == 1).simplify().is_true(); }
				}
				continue;
			}
			else if (res == z3::unknown) {
				bad_solve = 1;
				return;
			}
			sol.pop();
		}
	}
}

void ABVBitwiseInvariant::evaluate_to_best_Fix_Bilateral(double timeout) {
	set_timer(timeout);
	vector<int> s_upper;
	for (int i = 0; i < size; ++i) { s_upper.push_back(-1); }
	z3::solver sol(ctx);
	sol.push();
	sol.add(trans.pre);
	if (!bilateral_abstraction(sol)) {
		bad_solve = 1;
		return;
	}
	sol.pop();
	sol.add(trans.trans);
	while (true) {
		sol.push();
		sol.add(get_current_inv());
		if (!bilateral_abstraction_bar(sol)) { return; }
		sol.pop();
	}
}