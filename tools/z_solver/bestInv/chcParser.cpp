#include"chcParser.h"

using namespace std;

chcParser::chcParser()
	: ctx(*(new z3::context())) {
}

chcParser::~chcParser() {
	delete &ctx;
}

transitionSystem chcParser::file_to_transys(string filepath) {
	z3::solver sol(ctx);
	sol.from_file(filepath.c_str());
	
	z3::expr_vector vars(ctx);
	z3::expr_vector vars_bar(ctx);
	z3::expr const1 = sol.assertions()[0];
	z3::expr const2 = sol.assertions()[1];
	z3::expr const3 = sol.assertions()[2];

	int var_siz = Z3_get_quantifier_num_bound(ctx, const2) / 2;
	for (int i = 0; i < var_siz * 2; ++i) {
		z3::sort sort(ctx, Z3_get_quantifier_bound_sort(ctx, const2, i));
		string name = Z3_get_symbol_string(ctx, Z3_get_quantifier_bound_name(ctx, const2, i));
		if (i < var_siz) {
			vars.push_back(ctx.bv_const(name.c_str(), sort.bv_size()));
		}
		else {
			vars_bar.push_back(ctx.bv_const(name.c_str(), sort.bv_size()));
		}
	}

	z3::expr_vector var_v(ctx), all_v(ctx);
	for (int i = 0; i < var_siz; ++i) {
		var_v.push_back(vars[var_siz - i - 1]);
		all_v.push_back(vars_bar[var_siz - i - 1]);
	}
	for (int i = 0; i < var_siz; ++i) {
		all_v.push_back(vars[var_siz - i - 1]);
	}

	z3::expr pre = const1.body().substitute(var_v).arg(0);
	z3::expr trans = ctx.bool_val(true);
	for (unsigned i = 1; i < const2.body().arg(0).num_args(); ++i) {
		trans = trans && const2.body().substitute(all_v).arg(0).arg(i);
	}
	trans = trans.simplify();
	z3::expr post = const3.body().substitute(var_v).arg(1);

	return transitionSystem(ctx, pre, trans, post, vars, vars_bar);
}

transitionSystem chcParser::string_to_transys(string chc_problem) {
	// by LLM, to be checked
	z3::solver sol(ctx);
	sol.from_string(chc_problem.c_str());

	z3::expr_vector vars(ctx);
	z3::expr_vector vars_bar(ctx);
	z3::expr const1 = sol.assertions()[0];
	z3::expr const2 = sol.assertions()[1];
	z3::expr const3 = sol.assertions()[2];

	int var_siz = Z3_get_quantifier_num_bound(ctx, const2) / 2;
	for (int i = 0; i < var_siz * 2; ++i) {
		z3::sort sort(ctx, Z3_get_quantifier_bound_sort(ctx, const2, i));
		string name = Z3_get_symbol_string(ctx, Z3_get_quantifier_bound_name(ctx, const2, i));
	}

	z3::expr_vector var_v(ctx), all_v(ctx);
	for (int i = 0; i < var_siz; ++i) {
		var_v.push_back(vars[var_siz - i - 1]);
		all_v.push_back(vars_bar[var_siz - i - 1]);
	}
	for (int i = 0; i < var_siz; ++i) { all_v.push_back(vars[var_siz - i - 1]); }

	z3::expr pre = const1.body().substitute(var_v).arg(0);
	z3::expr trans = ctx.bool_val(true);
	for (unsigned i = 1; i < const2.body().arg(0).num_args(); ++i) {
		trans = trans && const2.body().substitute(all_v).arg(0).arg(i);
	}
	trans = trans.simplify();
	z3::expr post = const3.body().substitute(var_v).arg(1);

	return transitionSystem(ctx, pre, trans, post, vars, vars_bar);
}
