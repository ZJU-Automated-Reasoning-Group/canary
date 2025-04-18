#include<thread>
#include"invTemplate.h"
#include"utils/zUtils.h"
#include"parallelSolver/clusterSolver.h"
#include"modelSolver/assertionDatabase.h"

using namespace std;

UBVIntervalInvariant::UBVIntervalInvariant(transitionSystem trans)
    : invariant(trans) {
	size = trans.vars.size();
	for (int i = 0; i < size; ++i) {
		var.push_back(trans.vars[i]);
		var_bar.push_back(trans.vars_bar[i]);
		size_var.push_back(var.back().get_sort().bv_size());
		l_var.push_back(ctx.bv_const((var.back().to_string() + "_l").c_str(), size_var.back()));
		u_var.push_back(ctx.bv_const((var.back().to_string() + "_u").c_str(), size_var.back()));
		inv_cons = inv_cons && z3::ule(l_var.back(), var.back()) && z3::ule(var.back(), u_var.back());
		inv_bar_cons = inv_bar_cons && z3::ule(l_var.back(), var_bar.back()) && z3::ule(var_bar.back(), u_var.back());
	}
	inv_cons = inv_cons.simplify();
	inv_bar_cons = inv_bar_cons.simplify();
	pre_cons = z3::forall(trans.get_ori_consts(), z3::implies(trans.pre, inv_cons));
	trans_cons = z3::forall(trans.get_all_consts(), z3::implies(inv_cons && trans.trans, inv_bar_cons));
	post_cons = z3::forall(trans.get_ori_consts(), z3::implies(inv_cons, trans.post));
}

string UBVIntervalInvariant::l_val_string() {
	string ret;
	for (int i = 0; i < size; ++i) { ret += (i ? ' ' : '[') + l_val[i].to_string(); }
	return ret + ']';
}

string UBVIntervalInvariant::u_val_string() {
	string ret;
	for (int i = 0; i < size; ++i) { ret += (i ? ' ' : '[') + u_val[i].to_string(); }
	return ret + ']';
}

void UBVIntervalInvariant::initialize_inv_minmax() {
	for (int i = 0; i < size; ++i) {
		l_val.push_back(ctx.bv_val(0, size_var[i]));
		u_val.push_back(ctx.bv_val(-1, size_var[i]));
	}
}

z3::expr UBVIntervalInvariant::get_current_inv() {
	z3::expr cur_inv = ctx.bool_val(true);
	for (int i = 0; i < size; ++i) { cur_inv = cur_inv && z3::ule(l_val[i], var[i]) && z3::ule(var[i], u_val[i]); }
	return cur_inv.simplify();
}

z3::expr UBVIntervalInvariant::get_current_inv_bar() {
	z3::expr cur_inv = ctx.bool_val(true);
	for (int i = 0; i < size; ++i) { cur_inv = cur_inv && z3::ule(l_val[i], var_bar[i]) && z3::ule(var_bar[i], u_val[i]); }
	return cur_inv.simplify();
}

bool UBVIntervalInvariant::check_single_base(int& entry, z3::expr& l, z3::expr& u, z3::solver& sol) {
	sol.pop(), sol.push();
	sol.add(z3::ule(l, l_var[entry]) && z3::ule(u_var[entry], u));
	z3::check_result res = sol.check();
	if (timeout_check()) { return false; }
	++smt_count;

	if (res == z3::sat) { return true; }
	else if (res == z3::unknown) { bad_solve = 1; }
	return false;
}

bool UBVIntervalInvariant::check_single_model(int& entry, z3::expr& l, z3::expr& u, z3::solver& sol) {
	sol.pop(), sol.push();
	sol.add(z3::ule(l, l_var[entry]) && z3::ule(u_var[entry], u));
	z3::check_result res = sol.check();
	if (timeout_check()) { return false; }
	++smt_count;

	if (res == z3::sat) {
		z3::model model = sol.get_model();
		for (int i = 0; i < size; ++i) {
			l_val[i] = z3::max(l_val[i], model.eval(l_var[i], true)).simplify();
			u_val[i] = z3::min(u_val[i], model.eval(u_var[i], true)).simplify();
		}
		return true;
	}
	else if (res == z3::unknown) { bad_solve = 1; }
	return false;
}

bool UBVIntervalInvariant::check_single_bound(int& entry, z3::expr& l, z3::expr& u, z3::expr& lb, z3::expr& ub, z3::solver& sol) {
	sol.pop(), sol.push();
	sol.add(z3::ule(l, l_var[entry]) && z3::ule(l_var[entry], lb) && z3::ule(ub, u_var[entry]) && z3::ule(u_var[entry], u));
	z3::check_result res = sol.check();
	if (timeout_check()) { return false; }
	++smt_count;

	if (res == z3::sat) { return true; }
	else if (res == z3::unknown) { bad_solve = 1; }
	return false;
}

bool UBVIntervalInvariant::check_single_model_bound(int& entry, z3::expr& l, z3::expr& u, z3::expr& lb, z3::expr& ub, z3::solver& sol) {
	sol.pop(), sol.push();
	sol.add(z3::ule(l, l_var[entry]) && z3::ule(l_var[entry], lb) && z3::ule(ub, u_var[entry]) && z3::ule(u_var[entry], u));
	z3::check_result res = sol.check();
	if (timeout_check()) { return false; }
	++smt_count;

	if (res == z3::sat) {
		z3::model model = sol.get_model();
		for (int i = 0; i < size; ++i) {
			l_val[i] = z3::max(l_val[i], model.eval(l_var[i], true)).simplify();
			u_val[i] = z3::min(u_val[i], model.eval(u_var[i], true)).simplify();
		}
		return true;
	}
	else if (res == z3::unknown) { bad_solve = 1; }
	return false;
}

bool UBVIntervalInvariant::check_single_len(int& entry, z3::expr& len, z3::solver& sol) {
	sol.pop(), sol.push();
	sol.add(z3::ule(u_var[entry] - l_var[entry], len));
	z3::check_result res = sol.check();
	if (timeout_check()) { return false; }
	++smt_count;
	if (res == z3::sat) {
		z3::model model = sol.get_model();
		for (int i = 0; i < size; ++i) {
			l_val[i] = z3::max(l_val[i], model.eval(l_var[i], true)).simplify();
			u_val[i] = z3::min(u_val[i], model.eval(u_var[i], true)).simplify();
		}
		return true;
	}
	else if(res == z3::unknown) { bad_solve = 1; }
	return false;
}

bool UBVIntervalInvariant::check_total_len(z3::expr& cumu, z3::expr& len, z3::solver& sol) {
	sol.pop(), sol.push();
	sol.add(z3::ule(cumu, len));
	z3::check_result res = sol.check();
	if (timeout_check()) { return false; }
	++smt_count;
	if (res == z3::sat) { return true; }
	else if (res == z3::unknown) { bad_solve = 1; }
	return false;
}

bool UBVIntervalInvariant::search_for_minmax(z3::solver& sol) {
	for (int i = 0; i < size; ++i) {
		z3::expr l = ctx.bv_val(0, size_var[i]);
		z3::expr r = ctx.bv_val(-1, size_var[i]);
		while ((l != r).simplify().is_true()) {
			z3::expr mid = (l + z3::lshr(r - l, 1) + 1).simplify();
			sol.push();
			sol.add(z3::uge(var[i], mid));
			z3::check_result res = sol.check();
			if (timeout_check()) { return false; }
			++smt_count;
			if (res == z3::sat) { l = mid; }
			else if (res == z3::unsat) { r = (mid - 1).simplify(); }
			else {
				bad_solve = 1;
				return false;
			}
			sol.pop();
		}
		u_val.push_back(l);
		l = ctx.bv_val(0, size_var[i]);
		r = ctx.bv_val(-1, size_var[i]);
		while ((l != r).simplify().is_true()) {
			z3::expr mid = (l + z3::lshr(r - l, 1)).simplify();
			sol.push();
			sol.add(z3::ule(var[i], mid));
			z3::check_result res = sol.check();
			if (timeout_check()) { return false; }
			++smt_count;
			if (res == z3::sat) { r = mid; }
			else if (res == z3::unsat) { l = (mid + 1).simplify(); }
			else {
				bad_solve = 1;
				return false;
			}
			sol.pop();
		}
		l_val.push_back(l);
	}
	return true;
}

bool UBVIntervalInvariant::search_for_minmax_bar(z3::solver& sol) {
	z3::check_result res = sol.check();
	if (timeout_check()) { return false; }
	++smt_count;
	if (res != z3::sat) {
		if (res == z3::unknown) { bad_solve = 1; }
		return false;
	}
	z3::model model = sol.get_model();
	for (int i = 0; i < size; ++i) {
		z3::expr l = model.eval(var_bar[i], true);
		z3::expr r = ctx.bv_val(-1, size_var[i]);
		while ((l != r).simplify().is_true()) {
			z3::expr mid = (l + z3::lshr(r - l, 1) + 1).simplify();
			sol.push();
			sol.add(z3::uge(var_bar[i], mid));
			res = sol.check();
			if (timeout_check()) { return false; }
			++smt_count;
			if (res == z3::sat) { l = mid; }
			else if (res == z3::unsat) { r = (mid - 1).simplify(); }
			else {
				bad_solve = 1;
				return false;
			}
			sol.pop();
		}
		u_val[i] = z3::max(u_val[i], l).simplify();
		l = ctx.bv_val(0, size_var[i]);
		r = model.eval(var_bar[i], true);
		while ((l != r).simplify().is_true()) {
			z3::expr mid = (l + z3::lshr(r - l, 1)).simplify();
			sol.push();
			sol.add(z3::ule(var_bar[i], mid));
			res = sol.check();
			if (timeout_check()) { return false; }
			++smt_count;
			if (res == z3::sat) { r = mid; }
			else if (res == z3::unsat) { l = (mid + 1).simplify(); }
			else {
				bad_solve = 1;
				return false;
			}
			sol.pop();
		}
		l_val[i] = z3::min(l_val[i], l).simplify();
	}
	return true;
}

bool UBVIntervalInvariant::bilateral_abstraction(z3::solver& sol, vector<z3::expr>& l_lower, vector<z3::expr>& u_lower, vector<z3::expr>& l_new, vector<z3::expr>& u_new) {
	auto& l_upper = l_val;
	auto& u_upper = u_val;
	for (int i = 0; i < size; ++i) {
		l_upper.push_back(ctx.bv_val(0, size_var[i]));
		u_upper.push_back(ctx.bv_val(-1, size_var[i]));
		l_lower.push_back(ctx.bv_val(-1, size_var[i]));
		u_lower.push_back(ctx.bv_val(0, size_var[i]));
		l_new.push_back(z3::expr(ctx));
		u_new.push_back(z3::expr(ctx));
	}
	while (true) {
		if (!abstract_sequence(l_lower, l_upper, u_lower, u_upper, l_new, u_new)) { return true; }
		z3::expr lim = ctx.bool_val(true);
		for (int i = 0; i < size; ++i) { lim = lim && z3::ule(l_new[i], var[i]) && z3::ule(var[i], u_new[i]); }
		sol.push();
		sol.add(!lim);
		z3::check_result res = sol.check();
		if (timeout_check()) { return false; }
		++smt_count;
		if (res == z3::sat) {
			z3::model model = sol.get_model();
			for (int i = 0; i < size; ++i) {
				l_lower[i] = z3::min(l_lower[i], model.eval(var[i], true)).simplify();
				u_lower[i] = z3::max(u_lower[i], model.eval(var[i], true)).simplify();
			}
		}
		else if (res == z3::unsat) {
			for (int i = 0; i < size; ++i) {
				l_upper[i] = z3::max(l_upper[i], l_new[i]).simplify();
				u_upper[i] = z3::min(u_upper[i], u_new[i]).simplify();
			}
		}
		else {
			bad_solve = 1;
			return false;
		}
		sol.pop();
	}
}

bool UBVIntervalInvariant::bilateral_abstraction_bar(z3::solver& sol, vector<z3::expr>& l_lower, vector<z3::expr>& u_lower, vector<z3::expr>& l_new, vector<z3::expr>& u_new) {
	auto& l_upper = l_val;
	auto& u_upper = u_val;
	for (int i = 0; i < size; ++i) {
		l_upper[i] = ctx.bv_val(0, size_var[i]);
		u_upper[i] = ctx.bv_val(-1, size_var[i]);
	}
	bool tag = false;
	while (true) {
		if (!abstract_sequence(l_lower, l_upper, u_lower, u_upper, l_new, u_new)) { return tag; }
		z3::expr lim = ctx.bool_val(true);
		for (int i = 0; i < size; ++i) { lim = lim && z3::ule(l_new[i], var_bar[i]) && z3::ule(var_bar[i], u_new[i]); }
		sol.push();
		sol.add(!lim);
		z3::check_result res = sol.check();
		if (timeout_check()) { return false; }
		++smt_count;
		if (res == z3::sat) {
			z3::model model = sol.get_model();
			for (int i = 0; i < size; ++i) {
				l_lower[i] = z3::min(l_lower[i], model.eval(var_bar[i], true)).simplify();
				u_lower[i] = z3::max(u_lower[i], model.eval(var_bar[i], true)).simplify();
			}
			tag = true;
		}
		else if (res == z3::unsat) {
			for (int i = 0; i < size; ++i) {
				l_upper[i] = z3::max(l_upper[i], l_new[i]).simplify();
				u_upper[i] = z3::min(u_upper[i], u_new[i]).simplify();
			}
		}
		else {
			bad_solve = 1;
			return false;
		}
		sol.pop();
	}
}

bool UBVIntervalInvariant::abstract_sequence(std::vector<z3::expr>& l_lower, std::vector<z3::expr>& l_upper, std::vector<z3::expr>& u_lower, std::vector<z3::expr>& u_upper, std::vector<z3::expr>& l_new, std::vector<z3::expr>& u_new) {
	bool tag = false;
	for (int i = 0; i < size; ++i) {
		l_new[i] = (l_upper[i] + z3::lshr(l_lower[i] - l_upper[i], 1)).simplify();
		if ((l_new[i] != l_upper[i]).simplify().is_true()) { tag = true; }
		u_new[i] = (u_lower[i] + z3::lshr(u_upper[i] - u_lower[i], 1)).simplify();
		if ((u_new[i] != u_upper[i]).simplify().is_true()) { tag = true; }
	}
	if (!tag) {
		for (int i = 0; i < size; ++i) {
			if ((l_lower[i] != l_upper[i]).simplify().is_true()) {
				l_new[i] = (l_upper[i] + 1).simplify();
				tag = true;
				break;
			}
		}
	}
	return tag;
}

void UBVIntervalInvariant::evaluate_to_best_OMT(double timeout) {
	set_timer(timeout);
	z3::optimize sol(ctx);
	sol.add(pre_cons && trans_cons);
	for (int i = 0; i < size; ++i) {
		sol.maximize(l_var[i]);
		sol.minimize(u_var[i]);
	}
	z3::check_result res = sol.check();
	if (timeout_check()) { return; }
	omt_count += 2 * size;
	if (res != z3::sat) {
		bad_solve = 1;
		return;
	}
	z3::model model = sol.get_model();
	for (int i = 0; i < size; ++i) {
		l_val.push_back(model.eval(l_var[i], true));
		u_val.push_back(model.eval(u_var[i], true));
	}
}

void UBVIntervalInvariant::evaluate_to_best_Lift(double timeout) {
	set_timer(timeout);
	initialize_inv_minmax();
	z3::solver sol(ctx);
	sol.add(pre_cons && trans_cons);
	sol.push();
	for (int i = 0; i < size; ++i) {
		z3::expr& l = l_val[i];
		z3::expr& u = u_val[i];
		for (int j = size_var[i] - 1; j >= 0; --j) {
			z3::expr bit = z3::shl(ctx.bv_val(1, size_var[i]), j).simplify();
			z3::expr l_new = (l + bit).simplify();
			if ((z3::ule(l_new, u) && z3::ult(l, l_new)).simplify().is_true()) {
				if (check_single_base(i, l_new, u, sol)) { l = l_new; }
				else if (bad_solve) { return; }
			}
			z3::expr u_new = (u - bit).simplify();
			if ((z3::ule(l, u_new) && z3::ult(u_new, u)).simplify().is_true()) {
				if (check_single_base(i, l, u_new, sol)) { u = u_new; }
				else if (bad_solve) { return; }
			}
		}
	}
}

void UBVIntervalInvariant::evaluate_to_best_Lift_Model(double timeout) {
	set_timer(timeout);
	initialize_inv_minmax();
	z3::solver sol(ctx);
	sol.add(pre_cons && trans_cons);
	sol.push();
	for (int i = 0; i < size; ++i) {
		z3::expr& l = l_val[i];
		z3::expr& u = u_val[i];
		for (int j = size_var[i] - 1; j >= 0; --j) {
			z3::expr bit = z3::shl(ctx.bv_val(1, size_var[i]), j).simplify();
			z3::expr l_new = (l + bit).simplify();
			if ((z3::ule(l_new, u) && z3::ult(l, l_new)).simplify().is_true()) {
				check_single_model(i, l_new, u, sol);
				if (bad_solve) { return; }
			}
			z3::expr u_new = (u - bit).simplify();
			if ((z3::ule(l, u_new) && z3::ult(u_new, u)).simplify().is_true()) {
				check_single_model(i, l, u_new, sol);
				if (bad_solve) { return; }
			}
		}
	}
}

void UBVIntervalInvariant::evaluate_to_best_Lift_Bound(double timeout) {
	set_timer(timeout);
	initialize_inv_minmax();
	vector<z3::expr> lb_val, ub_val;
	for (int i = 0; i < size; ++i) {
		lb_val.push_back(u_val[i]);
		ub_val.push_back(l_val[i]);
	}
	z3::solver sol(ctx);
	sol.add(pre_cons && trans_cons);
	sol.push();
	z3::expr limt = ctx.bool_val(true);
	for (int i = 0; i < size; ++i) {
		z3::expr& l = l_val[i];
		z3::expr& u = u_val[i];
		z3::expr& lb = lb_val[i];
		z3::expr& ub = ub_val[i];
		for (int j = size_var[i] - 1; j >= 0; --j) {
			z3::expr bit = z3::shl(ctx.bv_val(1, size_var[i]), j).simplify();
			z3::expr l_new = (l + bit).simplify();
			if ((z3::ule(l_new, lb) && z3::ult(l, l_new)).simplify().is_true()) {
				if (check_single_bound(i, l_new, u, lb, ub, sol)) {
					l = l_new;
					ub = z3::max(l, ub).simplify();
				}
				else {
					if (bad_solve) { return; }
					lb = (l_new - 1).simplify();
				}
			}
			z3::expr u_new = (u - bit).simplify();
			if ((z3::ule(ub, u_new) && z3::ult(u_new, u)).simplify().is_true()) {
				if (check_single_bound(i, l, u_new, lb, ub, sol)) {
					u = u_new;
					lb = z3::min(u, lb).simplify();
				}
				else {
					if (bad_solve) { return; }
					ub = (u_new + 1).simplify();
				}
			}
		}
	}
}

void UBVIntervalInvariant::evaluate_to_best_Lift_Model_Bound(double timeout) {
	set_timer(timeout);
	initialize_inv_minmax();
	vector<z3::expr> lb_val, ub_val;
	for (int i = 0; i < size; ++i) {
		lb_val.push_back(u_val[i]);
		ub_val.push_back(l_val[i]);
	}
	z3::solver sol(ctx);
	sol.add(pre_cons && trans_cons);
	sol.push();
	z3::expr limt = ctx.bool_val(true);
	for (int i = 0; i < size; ++i) {
		z3::expr& l = l_val[i];
		z3::expr& u = u_val[i];
		z3::expr& lb = lb_val[i];
		z3::expr& ub = ub_val[i];
		for (int j = size_var[i] - 1; j >= 0; --j) {
			z3::expr bit = z3::shl(ctx.bv_val(1, size_var[i]), j).simplify();
			z3::expr l_new = (l + bit).simplify();
			if ((z3::ule(l_new, lb) && z3::ult(l, l_new)).simplify().is_true()) {
				if (check_single_model_bound(i, l_new, u, lb, ub, sol)) {
					ub = z3::max(l, ub).simplify();
				}
				else {
					if (bad_solve) { return; }
					lb = (l_new - 1).simplify();
				}
			}
			z3::expr u_new = (u - bit).simplify();
			if ((z3::ule(ub, u_new) && z3::ult(u_new, u)).simplify().is_true()) {
				if (check_single_model_bound(i, l, u_new, lb, ub, sol)) {
					lb = z3::min(u, lb).simplify();
				}
				else {
					if (bad_solve) { return; }
					ub = (u_new + 1).simplify();
				}
			}
		}
	}
}

void UBVIntervalInvariant::evaluate_to_best_Lift_Bound_Heu(double timeout) {
	set_timer(timeout);
	initialize_inv_minmax();
	z3::solver sol(ctx);
	sol.add(pre_cons && trans_cons);
	sol.push();
	while (true) {
		z3::expr limt = ctx.bool_val(true);
		for (int i = 0; i < size; ++i) {
			z3::expr& l = l_val[i];
			z3::expr& u = u_val[i];
			z3::expr lb = u_val[i];
			z3::expr ub = l_val[i];
			for (int j = size_var[i] - 1; j >= 0; --j) {
				if (z3::ule(lb, ub).simplify().is_true()) {
					limt = limt && z3::ule(l_var[i], lb) && z3::ule(ub, u_var[i]);
					break;
				}
				z3::expr bit = z3::shl(ctx.bv_val(1, size_var[i]), j).simplify();
				z3::expr l_new = (l + bit).simplify();
				if ((z3::ule(l_new, lb) && z3::ult(l, l_new)).simplify().is_true()) {
					if (check_single_bound(i, l_new, u, lb, ub, sol)) {
						l = l_new;
						ub = z3::max(l, ub).simplify();
					}
					else {
						if (bad_solve) { return; }
						lb = (l_new - 1).simplify();
					}
				}
				z3::expr u_new = (u - bit).simplify();
				if ((z3::ule(ub, u_new) && z3::ult(u_new, u)).simplify().is_true()) {
					if (check_single_bound(i, l, u_new, lb, ub, sol)) {
						u = u_new;
						lb = z3::min(u, lb).simplify();
					}
					else {
						if (bad_solve) { return; }
						ub = (u_new + 1).simplify();
					}
				}
			}
		}
		sol.pop();
		sol.add(limt);
		sol.push();
		z3::expr strict = ctx.bool_val(false);
		for (int i = 0; i < size; ++i) { strict = strict || z3::ult(l_val[i], l_var[i]) || z3::ult(u_var[i], u_val[i]); }
		sol.add(strict);
		z3::check_result res = sol.check();
		if (timeout_check()) { return; }
		++smt_count;
		if (res == z3::sat) {
			z3::model model = sol.get_model();
			for (int i = 0; i < size; ++i) {
				l_val[i] = z3::max(l_val[i], model.eval(l_var[i], true)).simplify();
				u_val[i] = z3::min(u_val[i], model.eval(u_var[i], true)).simplify();
			}
		}
		else {
			if (res == z3::unknown) { bad_solve = 1; }
			return;
		}
	}
}

void UBVIntervalInvariant::evaluate_to_best_Lift_Bound_FullHeu(double timeout) {
	set_timer(timeout);
	initialize_inv_minmax();
	vector<z3::expr> lb_val, ub_val;
	for (int i = 0; i < size; ++i) {
		lb_val.push_back(u_val[i]);
		ub_val.push_back(l_val[i]);
	}
	z3::solver sol(ctx);
	sol.add(pre_cons && trans_cons);
	sol.push();
	z3::expr limt = ctx.bool_val(true);
	for (int i = 0; i < size; ++i) {
		z3::expr& l = l_val[i];
		z3::expr& u = u_val[i];
		z3::expr& lb = lb_val[i];
		z3::expr& ub = ub_val[i];
		for (int j = size_var[i] - 1; j >= 0; --j) {
			if (z3::ule(lb, ub).simplify().is_true()) {
				limt = limt && z3::ule(l_var[i], lb) && z3::ule(ub, u_var[i]);
				break;
			}
			z3::expr bit = z3::shl(ctx.bv_val(1, size_var[i]), j).simplify();
			z3::expr l_new = (l + bit).simplify();
			if ((z3::ule(l_new, lb) && z3::ult(l, l_new)).simplify().is_true()) {
				if (check_single_bound(i, l_new, u, lb, ub, sol)) {
					l = l_new;
					ub = z3::max(l, ub).simplify();
				}
				else {
					if (bad_solve) { return; }
					lb = (l_new - 1).simplify();
				}
			}
			z3::expr u_new = (u - bit).simplify();
			if ((z3::ule(ub, u_new) && z3::ult(u_new, u)).simplify().is_true()) {
				if (check_single_bound(i, l, u_new, lb, ub, sol)) {
					u = u_new;
					lb = z3::min(u, lb).simplify();
				}
				else {
					if (bad_solve) { return; }
					ub = (u_new + 1).simplify();
				}
			}
		}
	}
	sol.pop();
	sol.add(limt);
	sol.push();
	z3::expr lim = ctx.bool_val(true);
	z3::expr strict = ctx.bool_val(false);
	for (int i = 0; i < size; ++i) {
		lim = lim && z3::ule(l_val[i], l_var[i]) && z3::ule(u_var[i], u_val[i]);
		strict = strict || z3::ult(l_val[i], l_var[i]) || z3::ult(u_var[i], u_val[i]);
	}
	sol.add(strict && lim);
	z3::check_result res = sol.check();
	if (timeout_check()) { return; }
	++smt_count;
	while (res == z3::sat) {
		z3::model model = sol.get_model();
		lim = ctx.bool_val(true), strict = ctx.bool_val(false);
		for (int i = 0; i < size; ++i) {
			l_val[i] = model.eval(l_var[i], true);
			u_val[i] = model.eval(u_var[i], true);
			lim = lim && z3::ule(l_val[i], l_var[i]) && z3::ule(u_var[i], u_val[i]);
			strict = strict || z3::ult(l_val[i], l_var[i]) || z3::ult(u_var[i], u_val[i]);
		}
		sol.add(strict && lim);
		res = sol.check();
		if (timeout_check()) { return; }
		++smt_count;
	}
	if (res == z3::unknown) { bad_solve = 1; }
}

void UBVIntervalInvariant::evaluate_to_best_Lift_Model_Bound_Heu(double timeout) {
	set_timer(timeout);
	initialize_inv_minmax();
	z3::solver sol(ctx);
	sol.add(pre_cons && trans_cons);
	sol.push();
	while (true) {
		z3::expr limt = ctx.bool_val(true);
		for (int i = 0; i < size; ++i) {
			z3::expr& l = l_val[i];
			z3::expr& u = u_val[i];
			z3::expr lb = u_val[i];
			z3::expr ub = l_val[i];
			for (int j = size_var[i] - 1; j >= 0; --j) {
				if (z3::ule(lb, ub).simplify().is_true()) {
					limt = limt && z3::ule(l_var[i], lb) && z3::ule(ub, u_var[i]);
					break;
				}
				z3::expr bit = z3::shl(ctx.bv_val(1, size_var[i]), j).simplify();
				z3::expr l_new = (l + bit).simplify();
				if ((z3::ule(l_new, lb) && z3::ult(l, l_new)).simplify().is_true()) {
					if (check_single_model_bound(i, l_new, u, lb, ub, sol)) { ub = z3::max(l, ub).simplify(); }
					else {
						if (bad_solve) { return; }
						lb = (l_new - 1).simplify();
					}
				}
				z3::expr u_new = (u - bit).simplify();
				if ((z3::ule(ub, u_new) && z3::ult(u_new, u)).simplify().is_true()) {
					if (check_single_model_bound(i, l, u_new, lb, ub, sol)) { lb = z3::min(u, lb).simplify(); }
					else {
						if (bad_solve) { return; }
						ub = (u_new + 1).simplify();
					}
				}
			}
		}
		sol.pop();
		sol.add(limt);
		sol.push();
		z3::expr strict = ctx.bool_val(false);
		for (int i = 0; i < size; ++i) { strict = strict || z3::ult(l_val[i], l_var[i]) || z3::ult(u_var[i], u_val[i]); }
		sol.add(strict);
		z3::check_result res = sol.check();
		if (timeout_check()) { return; }
		++smt_count;
		if (res == z3::sat) {
			z3::model model = sol.get_model();
			for (int i = 0; i < size; ++i) {
				l_val[i] = z3::max(l_val[i], model.eval(l_var[i], true)).simplify();
				u_val[i] = z3::min(u_val[i], model.eval(u_var[i], true)).simplify();
			}
		}
		else {
			if (res == z3::unknown) { bad_solve = 1; }
			return;
		}
	}
}

void UBVIntervalInvariant::evaluate_to_best_Lift_Model_Bound_FullHeu(double timeout) {
	set_timer(timeout);
	initialize_inv_minmax();
	vector<z3::expr> lb_val, ub_val;
	for (int i = 0; i < size; ++i) {
		lb_val.push_back(u_val[i]);
		ub_val.push_back(l_val[i]);
	}
	z3::solver sol(ctx);
	sol.add(pre_cons && trans_cons);
	sol.push();
	z3::expr limt = ctx.bool_val(true);
	for (int i = 0; i < size; ++i) {
		z3::expr& l = l_val[i];
		z3::expr& u = u_val[i];
		z3::expr& lb = lb_val[i];
		z3::expr& ub = ub_val[i];
		for (int j = size_var[i] - 1; j >= 0; --j) {
			if (z3::ule(lb, ub).simplify().is_true()) {
				limt = limt && z3::ule(l_var[i], lb) && z3::ule(ub, u_var[i]);
				break;
			}
			z3::expr bit = z3::shl(ctx.bv_val(1, size_var[i]), j).simplify();
			z3::expr l_new = (l + bit).simplify();
			if ((z3::ule(l_new, lb) && z3::ult(l, l_new)).simplify().is_true()) {
				if (check_single_model_bound(i, l_new, u, lb, ub, sol)) { ub = z3::max(l, ub).simplify(); }
				else {
					if (bad_solve) { return; }
					lb = (l_new - 1).simplify();
				}
			}
			z3::expr u_new = (u - bit).simplify();
			if ((z3::ule(ub, u_new) && z3::ult(u_new, u)).simplify().is_true()) {
				if (check_single_model_bound(i, l, u_new, lb, ub, sol)) { lb = z3::min(u, lb).simplify(); }
				else {
					if (bad_solve) { return; }
					ub = (u_new + 1).simplify();
				}
			}
		}
	}
	sol.pop();
	sol.add(limt);
	sol.push();
	z3::expr lim = ctx.bool_val(true);
	z3::expr strict = ctx.bool_val(false);
	for (int i = 0; i < size; ++i) {
		lim = lim && z3::ule(l_val[i], l_var[i]) && z3::ule(u_var[i], u_val[i]);
		strict = strict || z3::ult(l_val[i], l_var[i]) || z3::ult(u_var[i], u_val[i]);
	}
	sol.add(strict && lim);
	z3::check_result res = sol.check();
	if (timeout_check()) { return; }
	++smt_count;
	while (res == z3::sat) {
		z3::model model = sol.get_model();
		lim = ctx.bool_val(true), strict = ctx.bool_val(false);
		for (int i = 0; i < size; ++i) {
			l_val[i] = model.eval(l_var[i], true);
			u_val[i] = model.eval(u_var[i], true);
			lim = lim && z3::ule(l_val[i], l_var[i]) && z3::ule(u_var[i], u_val[i]);
			strict = strict || z3::ult(l_val[i], l_var[i]) || z3::ult(u_var[i], u_val[i]);
		}
		sol.add(strict && lim);
		res = sol.check();
		if (timeout_check()) { return; }
		++smt_count;
	}
	if (res == z3::unknown) { bad_solve = 1; }
}

void UBVIntervalInvariant::evaluate_to_best_Lift_Model_Bound_Heu_Parallel(double timeout) {
	set_timer(timeout);
	initialize_inv_minmax();
	clusterSolver sol(1);
	int base = assertionDatabase::add_assertion(1, pre_cons && trans_cons);
	while (true) {
		z3::expr limt = ctx.bool_val(true);
		vector<thread*> threads;
		std::mutex tlock, wrtlock;
		for (int i = 0; i < size; ++i) {
			generalExpr l = l_val[i];
			generalExpr u = u_val[i];
			generalExpr lb = u_val[i];
			generalExpr ub = l_val[i];
			vector<generalExpr> evaler;
			int size_var_i = size_var[i];
			for (int k = 0; k < size; ++k) {
				evaler.push_back(l_var[k]);
				evaler.push_back(u_var[k]);
			}
			generalExpr cons = ctx.bool_val(true);
			for (int j = 0; j < size; ++j) {
				generalExpr::ule(l_val[j], l_var[j]);
				cons = cons && generalExpr::ule(l_val[j], l_var[j]) && generalExpr::ule(u_var[j], u_val[j]);
			}
			cons = cons.simplify();
			threads.push_back(new thread([&, i](int entry) {
				printf("Thread for var %d starts\n", i);
				for (int j = size_var_i - 1; j >= 0; --j) {
					thread* lcheck = nullptr;
					thread* ucheck = nullptr;

					if (generalExpr::ule(lb, ub).simplify().is_true()) {
						tlock.lock();
						limt = limt && z3::ule(l_var[i], lb.expr_for_ctx(ctx)) && z3::ule(ub.expr_for_ctx(ctx), u_var[i]);
						tlock.unlock();
						break;
					}
					int cur = assertionDatabase::add_assertion(base, cons);
					generalExpr bit = z3::shl(ctx.bv_val(1, size_var_i), j).simplify();

					generalExpr l_new = (l + bit).simplify();
					if ((generalExpr::ule(l_new, lb) && generalExpr::ult(l, l_new)).simplify().is_true()) {
						int curl = assertionDatabase::add_assertion(cur, generalExpr::ule(l_new, l_var[entry]));
						lcheck = new thread([&]() {
							printf("Thread for var %d lcheck starts\n", i); 
							checkResults res = sol.incre_solve(curl, evaler);
							if (res.res == z3::sat) {
								for (int i = 0; i < size; ++i) {
									wrtlock.lock();
									l_val[i] = res.eval[i * 2].expr_for_ctx(ctx);
									u_val[i] = res.eval[i * 2 + 1].expr_for_ctx(ctx);
									wrtlock.unlock();
								}
							}
							else if (res.res == z3::unknown) { bad_solve = 1; }
						});
					}
					generalExpr u_new = (u - bit).simplify();
					if ((generalExpr::ule(ub, u_new) && generalExpr::ult(u_new, u)).simplify().is_true()) {
						int curr = assertionDatabase::add_assertion(cur, generalExpr::ule(u_var[entry], u_new));
						ucheck = new thread([&]() { 
							printf("Thread for var %d ucheck starts\n", i); 
							checkResults res = sol.incre_solve(curr, evaler);
							if (res.res == z3::sat) {
								for (int i = 0; i < size; ++i) {
									wrtlock.lock();
									l_val[i] = res.eval[i * 2].expr_for_ctx(ctx);
									u_val[i] = res.eval[i * 2 + 1].expr_for_ctx(ctx);
									wrtlock.unlock();
								}
							}
							else if (res.res == z3::unknown) { bad_solve = 1; }
						});
					}
					if (lcheck) {
						lcheck->join();
						++smt_count;
						delete lcheck;
					}
					if (ucheck) {
						ucheck->join();
						++smt_count;
						delete ucheck;
					}
				}
			} , i));
		}
		for (int i = 0; i < threads.size(); ++i) {
			threads[i]->join();
			delete threads[i];
		}
		if (timeout_check()) { return; }

		z3::solver finsol(ctx);
		z3::expr lim = ctx.bool_val(true);
		z3::expr strict = ctx.bool_val(false);
		for (int i = 0; i < size; ++i) {
			lim = lim && z3::ule(l_val[i], l_var[i]) && z3::ule(u_var[i], u_val[i]);
			strict = strict || z3::ult(l_val[i], l_var[i]) || z3::ult(u_var[i], u_val[i]);
		}
		finsol.add(pre_cons && trans_cons && limt && strict && lim);
		z3::check_result res = finsol.check();
		if (timeout_check()) { return; }
		++smt_count;
		if (res == z3::sat) {
			z3::model model = finsol.get_model();
			for (int i = 0; i < size; ++i) {
				l_val[i] = model.eval(l_var[i], true);
				u_val[i] = model.eval(u_var[i], true);
			}
		}
		else {
			if (res == z3::unknown) { bad_solve = 1; }
			return;
		}
	}
}

void UBVIntervalInvariant::evaluate_to_best_Decr(double timeout) {
	set_timer(timeout);
	initialize_inv_minmax();

	z3::expr strict = ctx.bool_val(false);
	for (int i = 0; i < size; ++i) { strict = strict || z3::ult(l_val[i], l_var[i]) || z3::ult(u_var[i], u_val[i]); }

	z3::solver sol(ctx);
	sol.add(pre_cons && trans_cons);
	sol.push();
	sol.add(strict);
	z3::check_result res = sol.check();
	if (timeout_check()) { return; }
	++smt_count;
	if (res == z3::unsat) {
		bad_solve = 1;
		return;
	}
	while (res == z3::sat) {
		z3::model model = sol.get_model();
		strict = ctx.bool_val(false);
		for (int i = 0; i < size; ++i) {
			l_val[i] = z3::max(l_val[i], model.eval(l_var[i], true)).simplify();
			u_val[i] = z3::min(u_val[i], model.eval(u_var[i], true)).simplify();
			strict = strict || z3::ult(l_val[i], l_var[i]) || z3::ult(u_var[i], u_val[i]);
		}

		sol.pop(), sol.push();
		sol.add(strict);
		res = sol.check();
		if (timeout_check()) { return; }
		++smt_count;
	}
	if (res == z3::unknown) { bad_solve = 1; }
}

void UBVIntervalInvariant::evaluate_to_best_Len_Single(double timeout) {
	set_timer(timeout);
	initialize_inv_minmax();
	z3::solver sol(ctx);
	sol.add(pre_cons && trans_cons);
	sol.push();
	for (int i = 0; i < size; ++i) {
		z3::expr lenl = l_val[i];
		z3::expr lenu = u_val[i];
		while ((lenl != lenu).simplify().is_true()) {
			z3::expr mid = (lenl + z3::lshr(lenu - lenl, 1)).simplify();
			if (check_single_len(i, mid, sol)) { lenu = (u_val[i] - l_val[i]).simplify(); }
			else {
				if (bad_solve) { return; }
				lenl = (mid + 1).simplify();
			}
		}
	}
}

void UBVIntervalInvariant::evaluate_to_best_Len_Total(double timeout) {
	set_timer(timeout);
	initialize_inv_minmax();
	int maxlen = 0;
	for (int i = 0; i < size; ++i) { maxlen = max(maxlen, size_var[i]); }
	int cumulen = zUtils::shift_count_lt32(size) + maxlen;
	z3::expr cumu = ctx.bv_const("cumu_len", cumulen);
	z3::expr cumi = ctx.bv_val(0, cumulen);
	for (int i = 0; i < size; ++i) { cumi = zUtils::unaligned_bvadd(cumi, u_var[i] - l_var[i]); }
	z3::solver sol(ctx);
	sol.add(pre_cons && trans_cons);
	sol.add((cumi == cumu).simplify());
	sol.push();
	z3::expr lenl = ctx.bv_val(0, cumulen);
	z3::expr lenu = ctx.bv_val(-1, cumulen);
	while ((lenl != lenu).simplify().is_true()) {
		z3::expr mid = (lenl + z3::lshr(lenu - lenl, 1)).simplify();
		if (check_total_len(cumu, mid, sol)) {
			z3::model model = sol.get_model();
			for (int i = 0; i < size; ++i) {
				l_val[i] = z3::max(l_val[i], model.eval(l_var[i], true)).simplify();
				u_val[i] = z3::min(u_val[i], model.eval(u_var[i], true)).simplify();
			}
			lenu = model.eval(cumu, true);
		}
		else {
			if (bad_solve) { return; }
			lenl = (mid + 1).simplify();
		}
	}
}

void UBVIntervalInvariant::evaluate_to_best_Fix_Single(double timeout) {
	set_timer(timeout);
	z3::solver sol1(ctx), sol2(ctx);
	sol1.add(trans.pre);
	z3::check_result res = sol1.check();
	if (timeout_check()) { return; }
	++smt_count;
	if (res != z3::sat) {
		bad_solve = 1;
		return;
	}
	z3::model model = sol1.get_model();
	for (int i = 0; i < size; ++i) {
		l_val.push_back(model.eval(var[i], true).simplify());
		u_val.push_back(model.eval(var[i], true).simplify());
	}
	sol1.push();
	sol1.add(!get_current_inv());
	res = sol1.check();
	if (timeout_check()) { return; }
	++smt_count;
	while (res == z3::sat) {
		model = sol1.get_model();
		for (int i = 0; i < size; ++i) {
			l_val[i] = z3::min(model.eval(var[i], true), l_val[i]).simplify();
			u_val[i] = z3::max(model.eval(var[i], true), u_val[i]).simplify();
		}
		sol1.pop(), sol1.push();
		sol1.add(!get_current_inv());
		sol1.check();
		if (timeout_check()) { return; }
		++smt_count;
	}
	if (res == z3::unknown) {
		bad_solve = 1;
		return;
	}
	sol2.add(trans.trans);
	sol2.push();
	sol2.add(get_current_inv() && !get_current_inv_bar());
	res = sol2.check();
	if (timeout_check()) { return; }
	++smt_count;
	while (res == z3::sat) {
		model = sol2.get_model();
		for (int i = 0; i < size; ++i) {
			l_val[i] = z3::min(model.eval(var_bar[i], true), l_val[i]).simplify();
			u_val[i] = z3::max(model.eval(var_bar[i], true), u_val[i]).simplify();
		}
		sol2.pop(), sol2.push();
		sol2.add(get_current_inv() && !get_current_inv_bar());
		res = sol2.check();
		if (timeout_check()) { return; }
		++smt_count;
	}
	if (res == z3::unknown) {
		bad_solve = 1;
		return;
	}
}

void UBVIntervalInvariant::evaluate_to_best_Fix_OMT(double timeout) {
	set_timer(timeout);
	z3::optimize opt(ctx);
	opt.push();
	opt.add(trans.pre);
	z3::check_result res;
	for (int i = 0; i < size; ++i) {
		opt.push();
		opt.maximize(var[i]);
		res = opt.check();
		if (timeout_check()) { return; }
		++omt_count;
		if (res != z3::sat) {
			bad_solve = 1;
			return;
		}
		u_val.push_back(opt.get_model().eval(var[i], true));
		opt.pop(), opt.push();
		opt.minimize(var[i]);
		res = opt.check();
		if (timeout_check()) { return; }
		++omt_count;
		if (res != z3::sat) {
			bad_solve = 1;
			return;
		}
		l_val.push_back(opt.get_model().eval(var[i], true));
		opt.pop();
	}
	opt.pop();
	opt.add(trans.trans);
	while (true) {
		opt.push();
		opt.add(get_current_inv() && !get_current_inv_bar());
		for (int i = 0; i < size; ++i) {
			opt.push();
			opt.maximize(var_bar[i]);
			res = opt.check();
			if (timeout_check()) { return; }
			++omt_count;
			if (res != z3::sat) {
				if (res == z3::unknown) { bad_solve = 1; }
				return;
			}
			z3::expr updater = opt.get_model().eval(var_bar[i], true);
			if (z3::ult(u_val[i], updater).simplify().is_true()) {
				u_val[i] = updater;
				opt.pop();
				break;
			}
			opt.pop(), opt.push();
			opt.minimize(var_bar[i]);
			res = opt.check();
			if (timeout_check()) { return; }
			++omt_count;
			if (res != z3::sat) {
				if (res == z3::unknown) { bad_solve = 1; }
				return;
			}
			updater = opt.get_model().eval(var_bar[i], true);
			if (z3::ult(updater, l_val[i]).simplify().is_true()) {
				l_val[i] = updater;
				opt.pop();
				break;
			}
			opt.pop();
		}
		opt.pop();
	}
}

void UBVIntervalInvariant::evaluate_to_best_Fix_BS(double timeout) {
	set_timer(timeout);
	z3::solver sol(ctx);
	sol.push();
	sol.add(trans.pre);
	if (!search_for_minmax(sol)) {
		bad_solve = 1;
		return;
	}
	sol.pop();
	sol.add(trans.trans);
	while (true) {
		sol.push();
		sol.add(get_current_inv() && !get_current_inv_bar());
		if (!search_for_minmax_bar(sol)) { return; }
		sol.pop();
	}
}

void UBVIntervalInvariant::evaluate_to_best_Fix_Bilateral(double timeout) {
	set_timer(timeout);
	vector<z3::expr> l_lower, u_lower, l_new, u_new;
	z3::solver sol(ctx);
	sol.push();
	sol.add(trans.pre);
	if (!bilateral_abstraction(sol, l_lower, u_lower, l_new, u_new)) {
		bad_solve = 1;
		return;
	}
	sol.pop();
	sol.add(trans.trans);
	while (true) {
		sol.push();
		sol.add(get_current_inv());
		if (!bilateral_abstraction_bar(sol, l_lower, u_lower, l_new, u_new)) { return; }
		sol.pop();
	}
}
