#pragma once
#include"z3++.h"

struct transitionSystem {
	z3::context& ctx;
	z3::expr pre;
	z3::expr trans;
	z3::expr post;
	z3::expr_vector vars;
	z3::expr_vector vars_bar;

	transitionSystem(z3::context& ctx, z3::expr pre, z3::expr trans, z3::expr post, z3::expr_vector vars, z3::expr_vector vars_bar);

	z3::expr_vector get_ori_consts();
	z3::expr_vector get_bar_consts();
	z3::expr_vector get_all_consts();
};