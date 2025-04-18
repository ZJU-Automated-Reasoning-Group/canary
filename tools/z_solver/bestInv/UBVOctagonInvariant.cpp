#include"invTemplate.h"
#include"utils/zUtils.h"

using namespace std;

UBVOctagonInvariant::UBVOctagonInvariant(transitionSystem trans)
    : invariant(trans) {
	size = trans.vars.size() * trans.vars.size();
	for (int i = 0; i < trans.vars.size(); ++i) {
		var.push_back(trans.vars[i]);
		var_bar.push_back(trans.vars_bar[i]);
		size_var.push_back(var.back().get_sort().bv_size());
		l_var.push_back(ctx.bv_const((var.back().to_string() + "_l").c_str(), size_var.back()));
		u_var.push_back(ctx.bv_const((var.back().to_string() + "_u").c_str(), size_var.back()));
		inv_cons = inv_cons && z3::ule(l_var.back(), var.back()) && z3::ule(var.back(), u_var.back());
		inv_bar_cons = inv_bar_cons && z3::ule(l_var.back(), var_bar.back()) && z3::ule(var_bar.back(), u_var.back());
	}
	for (int i = 0; i < trans.vars.size(); ++i) {
		for (int j = 0; j < trans.vars.size(); ++j) {
			if (i != j) {
				string comb = var[i].to_string() + "_" + var[j].to_string() + "_";
				int siz = max(size_var[i], size_var[j]);
				l_var.push_back(ctx.bv_const((comb + "l").c_str(), siz));
				u_var.push_back(ctx.bv_const((comb + "u").c_str(), siz));
				size_var.push_back(siz);
				if (i < j) {
					z3::expr rlvar = zUtils::unaligned_bvadd(var[i], var[j]);
					z3::expr rlvar_bar = zUtils::unaligned_bvadd(var_bar[i], var_bar[j]);
					var.push_back(rlvar);
					var_bar.push_back(rlvar_bar);
					inv_cons = inv_cons && z3::ule(l_var.back(), rlvar) && z3::ule(rlvar, u_var.back());
					inv_bar_cons = inv_bar_cons && z3::ule(l_var.back(), rlvar_bar) && z3::ule(rlvar_bar, u_var.back());
				}
				else {
					z3::expr rlvar = zUtils::unaligned_bvadd(var[i], -var[j]);
					z3::expr rlvar_bar = zUtils::unaligned_bvadd(var_bar[i], -var_bar[j]);
					var.push_back(rlvar);
					var_bar.push_back(rlvar_bar);
					inv_cons = inv_cons && z3::ule(l_var.back(), rlvar) && z3::ule(rlvar, u_var.back());
					inv_bar_cons = inv_bar_cons && z3::ule(l_var.back(), rlvar_bar) && z3::ule(rlvar_bar, u_var.back());
				}
			}
		}
	}
	inv_cons = inv_cons.simplify();
	inv_bar_cons = inv_bar_cons.simplify();
	pre_cons = z3::forall(trans.get_ori_consts(), z3::implies(trans.pre, inv_cons));
	trans_cons = z3::forall(trans.get_all_consts(), z3::implies(inv_cons && trans.trans, inv_bar_cons));
	post_cons = z3::forall(trans.get_ori_consts(), z3::implies(inv_cons, trans.post));
}

string UBVOctagonInvariant::l_val_string() {
	string ret;
	for (int i = 0; i < size; ++i) { ret += (i ? ' ' : '[') + l_val[i].to_string(); }
	return ret + ']';
}

string UBVOctagonInvariant::u_val_string() {
	string ret;
	for (int i = 0; i < size; ++i) { ret += (i ? ' ' : '[') + u_val[i].to_string(); }
	return ret + ']';
}

void UBVOctagonInvariant::initialize_inv_minmax() {
	for (int i = 0; i < size; ++i) {
		l_val.push_back(ctx.bv_val(0, size_var[i]));
		u_val.push_back(ctx.bv_val(-1, size_var[i]));
	}
}

z3::expr UBVOctagonInvariant::get_current_inv() {
	z3::expr cur_inv = ctx.bool_val(true);
	for (int i = 0; i < size; ++i) { cur_inv = cur_inv && z3::ule(l_val[i], var[i]) && z3::ule(var[i], u_val[i]); }
	return cur_inv.simplify();
}

z3::expr UBVOctagonInvariant::get_current_inv_bar() {
	z3::expr cur_inv = ctx.bool_val(true);
	for (int i = 0; i < size; ++i) { cur_inv = cur_inv && z3::ule(l_val[i], var_bar[i]) && z3::ule(var_bar[i], u_val[i]); }
	return cur_inv.simplify();
}

bool UBVOctagonInvariant::check_single_model_bound(int& entry, z3::expr& l, z3::expr& u, z3::expr& lb, z3::expr& ub, z3::solver& sol) {
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

bool UBVOctagonInvariant::search_for_minmax(z3::solver& sol) {
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

bool UBVOctagonInvariant::search_for_minmax_bar(z3::solver& sol) {
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

bool UBVOctagonInvariant::bilateral_abstraction(z3::solver& sol, vector<z3::expr>& l_lower, vector<z3::expr>& u_lower, vector<z3::expr>& l_new, vector<z3::expr>& u_new) {
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

bool UBVOctagonInvariant::bilateral_abstraction_bar(z3::solver& sol, vector<z3::expr>& l_lower, vector<z3::expr>& u_lower, vector<z3::expr>& l_new, vector<z3::expr>& u_new) {
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

bool UBVOctagonInvariant::abstract_sequence(std::vector<z3::expr>& l_lower, std::vector<z3::expr>& l_upper, std::vector<z3::expr>& u_lower, std::vector<z3::expr>& u_upper, std::vector<z3::expr>& l_new, std::vector<z3::expr>& u_new) {
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

void UBVOctagonInvariant::evaluate_to_best_OMT(double timeout) {
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

void UBVOctagonInvariant::evaluate_to_best_Lift_Model_Bound_Heu(double timeout) {
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

void UBVOctagonInvariant::evaluate_to_best_Decr(double timeout) {
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

void UBVOctagonInvariant::evaluate_to_best_Fix_BS(double timeout) {
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

void UBVOctagonInvariant::evaluate_to_best_Fix_Bilateral(double timeout) {
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
