#include<random>
#include"zUtils.h"

int zUtils::shift_count_lt32(const int& x) {
	if (x <= 1) {
		return 0;
	}
	if (x <= 2) {
		return 1;
	}
	if (x <= 4) {
		return 2;
	}
	if (x <= 8) {
		return 3;
	}
	if (x <= 16) {
		return 4;
	}
	return 5;
}

z3::expr zUtils::unaligned_bvadd(const z3::expr& a, const z3::expr& b) {
	if (a.get_sort().bv_size() == b.get_sort().bv_size()) {
		return (a + b).simplify();
	}
	else if (a.get_sort().bv_size() < b.get_sort().bv_size()) {
		return (z3::concat(a.ctx().bv_val(0, b.get_sort().bv_size() - a.get_sort().bv_size()), a) + b).simplify();
	}
	else {
		return (a + z3::concat(a.ctx().bv_val(0, a.get_sort().bv_size() - b.get_sort().bv_size()), b)).simplify();
	}
}

z3::expr zUtils::get_random_bv(const z3::expr& l, const z3::expr& r, const bool& close_l, const bool& close_r) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(0, 1);
	z3::expr shift = (r - l).simplify();
	int loc = -1;
	int size = shift.get_sort().bv_size();
	for (int i = size - 1; i >= 0; --i) {
		z3::expr bit = z3::shl(shift.ctx().bv_val(1, size), i).simplify();
		if (z3::ult(bit, shift).simplify().is_true()) {
			loc = i;
			break;
		}
	}
	if (loc == -1) {
		return l;
	}
	while (true) {
		z3::expr zero = shift.ctx().bv_val(0, size);
		for (int i = 0; i <= loc; ++i) {
			if (dist(gen) == 1) {
				z3::expr bit = z3::shl(shift.ctx().bv_val(1, size), i).simplify();
				zero = (zero + bit).simplify();
			}
		}
		if ((z3::ult(0, zero) && z3::ult(zero, shift)).simplify().is_true()) {
			return (l + zero).simplify();
		}
		else if (close_l && (zero == 0).simplify().is_true()) {
			return l.simplify();
		}
		else if (close_r && (zero == shift).simplify().is_true()) {
			return r.simplify();
		}
	}
}

z3::expr_vector zUtils::reverse(const z3::expr_vector& vec) {
	z3::expr_vector ret(vec.ctx());
	for (int i = 0; i < vec.size(); ++i) {
		ret.push_back(vec[vec.size() - i - 1]);
	}
	return ret;
}

z3::expr_vector zUtils::append(const z3::expr_vector& a, const z3::expr_vector& b) {
	z3::expr_vector ret(a.ctx());
	for (int i = 0; i < a.size(); ++i) { ret.push_back(a[i]); }
	for (int i = 0; i < b.size(); ++i) { ret.push_back(b[i]); }
	return ret;
}