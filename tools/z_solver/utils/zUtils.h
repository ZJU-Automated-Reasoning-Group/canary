#pragma once
#include"z3++.h"

namespace zUtils {
	int shift_count_lt32(const int& x);
	z3::expr unaligned_bvadd(const z3::expr& a, const z3::expr& b);
	z3::expr get_random_bv(const z3::expr& l, const z3::expr& r, const bool& close_l, const bool & close_r);
	z3::expr_vector reverse(const z3::expr_vector& vec);
	z3::expr_vector append(const z3::expr_vector& a, const z3::expr_vector& b);
}