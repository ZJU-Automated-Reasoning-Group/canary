#include<ctime>
#include<iostream>
#include"csvDealer.h"
#include"parameterParser.h"
#include"bestInv/chcParser.h"
#include"bestInv/invTemplate.h"
#include"kInduction/kInductor.h"

using namespace std;
using namespace kInduction;

chcParser par;

double timestamp() {
    static double pre = 0;
    double now = double(clock()) / CLOCKS_PER_SEC;
    double ret = now - pre;
    pre = now;
    return ret;
}

void k_induction_test() {
    z3::context ctx;
    z3::expr x = ctx.int_const("x");
    z3::expr x_bar = ctx.int_const("x_bar");
    z3::expr i = ctx.int_const("i");
    z3::expr i_bar = ctx.int_const("i_bar");
    z3::expr n = ctx.int_const("n");
    z3::expr n_bar = ctx.int_const("n_bar");
    z3::expr pre = (x == 0 && i == 0 && n == 0);
    z3::expr trans = (z3::ite(n % 2 == 0, x_bar == x + 1 && i_bar == i, x_bar == x && i_bar == i + 1) && n_bar == n + 1);
    z3::expr post = (z3::implies(n % 2 == 0, x == i));
    z3::expr_vector vars(ctx), vars_bar(ctx);
    vars.push_back(x), vars.push_back(i), vars.push_back(n);
    vars_bar.push_back(x_bar), vars_bar.push_back(i_bar), vars_bar.push_back(n_bar);
    transitionSystem transys(ctx, pre, trans, post, vars, vars_bar);

    kInductor kind(transys);
    for (int i = 1; i <= 64; ++i) {
        printf("k = %d trying\n", i);
        if (kind.try_induct(i, ctx.bool_val(true))) {
            printf("KInd with k = %d succeed\n", i);
            break;
        }
    }
}

int main(int argc, char** argv, char** env) {

#if WIN32
    transitionSystem ts = par.file_to_transys("D:/VSProjects/z_solver/data/nove/128nia/s11.desugared.sl_128bits_unsigned.smt2");
	csvDealer csv("D:/VSProjects/a.csv");
    timestamp();
	UBVIntervalInvariant inv(ts);
	inv.evaluate_to_best_Lift_Model_Bound_Heu(2);
	int kreach = 0;
	kInductor kind(ts);
	double et = 0, st = 0;
	if (!inv.bad_solve) {
		st = timestamp();
		for (int i = 1; i <= 64; ++i) {
			if (kind.try_induct(i, inv.get_current_inv())) {
				kreach = i;
				break;
			}
		}
		et = timestamp();
	}
	printf("%d\n", kreach);
	csv.write_append(csvLine("", "", kind.proved, kind.result, kreach, et - st));
#else
    try {
        parameterParser paras(argc, argv);
        csvDealer csv(paras.output);

        paras.klim = 64;

        transitionSystem ts = par.file_to_transys(paras.file);

		int kreach = 0;
        double st = 0, et = 0;

		kInductor kind(ts);
		if (paras.domain == "interval") {
			UBVIntervalInvariant inv(ts);
			if (paras.method == "decr") { inv.evaluate_to_best_Decr(paras.timeout);}
			else if (paras.method == "optbin") { inv.evaluate_to_best_Lift_Model_Bound_Heu(paras.timeout); }
			else if (paras.method == "bi") { inv.evaluate_to_best_Fix_Bilateral(paras.timeout); }
			//if (!inv.bad_solve) {
				st = double(clock()) / CLOCKS_PER_SEC;
				for (int i = 1; i <= paras.klim; ++i) {
					bool res = kind.try_induct(i, inv.get_current_inv());
					if (res) {
						kreach = i;
						break;
					}
				}
				et = double(clock()) / CLOCKS_PER_SEC;
			//}
		}
        else if (paras.domain == "none") {
			st = double(clock()) / CLOCKS_PER_SEC;
			for (int i = 1; i <= paras.klim; ++i) {
				bool res = kind.try_induct(i, ts.ctx.bool_val(true));
				if (res) {
					kreach = i;
					break;
				}
			}
			et = double(clock()) / CLOCKS_PER_SEC;
        }
		csv.write_append(csvLine(paras.file, paras.method, kind.proved, kind.result, kreach, et - st));
    }
    catch (string str) {
        cerr << "Error: " + str << endl;
    }
#endif
    return 0;
}