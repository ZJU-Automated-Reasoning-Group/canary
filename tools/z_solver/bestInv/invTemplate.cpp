#include"invTemplate.h"

using namespace std;

invariant::invariant(transitionSystem trans)
	: ctx(trans.ctx), trans(trans), smt_count(0), omt_count(0), bad_solve(0), inv_cons(ctx.bool_val(true)), inv_bar_cons(ctx.bool_val(true)), pre_cons(ctx), trans_cons(ctx), post_cons(ctx), time_start(0), time_out(0) {
}

void invariant::set_timer(double timeout) {
	time_start = clock();
	time_out = timeout;
}

bool invariant::timeout_check() {
	if (time_out != -1 && clock() - time_start > time_out * CLOCKS_PER_SEC) {
		bad_solve = 2;
		return true;
	}
	return false;
}