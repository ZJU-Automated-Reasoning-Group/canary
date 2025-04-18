#pragma once
#include<string>
#include<map>
#include"bestInv/transitionSystem.h"
#include"z3++.h"

struct kInductor {
	z3::context& ctx;
	transitionSystem trans;
	bool result;
	bool proved;
	int k_last;
	int bad_solve; // 1 for unknown

	kInductor(transitionSystem trans);

	bool try_induct(int k, z3::expr inv);

	z3::expr_vector ite_vars(z3::expr_vector cur);
	z3::expr post_for(z3::expr_vector cur);
	z3::expr trans_for(z3::expr_vector cur, z3::expr_vector nxt);
};