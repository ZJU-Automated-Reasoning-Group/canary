#include"kInductor.h"
#include <utils/zUtils.h>

using namespace std;

kInductor::kInductor(transitionSystem trans)
	:ctx(trans.ctx), trans(trans), result(false), proved(false), bad_solve(0), k_last(0) {
	z3::solver sol(ctx);
	sol.add(trans.pre && !trans.post);
	z3::check_result res = sol.check();
	if (res == z3::sat) { bad_solve = 2; }
	else if (res == z3::unknown) { bad_solve = 1; }
}

bool kInductor::try_induct(int k, z3::expr inv) {
	if (bad_solve || proved) { return result; }
	z3::solver sol(ctx);
	z3::expr_vector cur = trans.vars;
	z3::expr invs = inv;
	z3::expr sustain_steps = ctx.bool_val(true);
	z3::expr sustain_proof = ctx.bool_val(true);
	sol.push();
	for (int i = 0; i < k; ++i) {
		z3::expr_vector nxt = ite_vars(cur);
		if (i == 0) { sol.add(trans.pre); }
		else if (i > k_last) {
			sol.push();
			sol.add(!post_for(cur));
			z3::check_result res = sol.check();
			if (res != z3::unsat) {
				if (res == z3::unknown) { bad_solve = 1; }
				else { proved = true; }
				return false;
			}
			sol.pop();
		}
		z3::expr nov_trans = trans_for(cur, nxt);
		sol.add(nov_trans);
		invs = (invs && inv.substitute(trans.vars, nxt)).simplify();
		sustain_steps = (sustain_steps && nov_trans).simplify();
		sustain_proof = (sustain_proof && post_for(cur)).simplify();
		cur = nxt;
	}
	k_last = k;
	sol.pop();
	sol.add(!z3::implies(invs && sustain_steps && sustain_proof, trans.post.substitute(trans.vars, cur)));
	z3::check_result res = sol.check();
	if (res == z3::unsat) { return result = proved = true; }
	else if (res == z3::unknown) { bad_solve = 1; }
	return false;
}

z3::expr_vector kInductor::ite_vars(z3::expr_vector cur) {
	z3::expr_vector ret(ctx);
	for (int i = 0; i < cur.size(); ++i) {
		if (cur[i].is_bv()) { ret.push_back(ctx.bv_const((cur[i].to_string() + "_").c_str(), cur[i].get_sort().bv_size())); }
		else if (cur[i].is_int()) { ret.push_back(ctx.int_const((cur[i].to_string() + "_").c_str())); }
	}
	return ret;
}

z3::expr kInductor::post_for(z3::expr_vector cur) {
	return trans.post.substitute(trans.vars, cur).simplify();
}

z3::expr kInductor::trans_for(z3::expr_vector cur, z3::expr_vector nxt) {
	return trans.trans.substitute(zUtils::append(trans.vars, trans.vars_bar), zUtils::append(cur, nxt)).simplify();
}