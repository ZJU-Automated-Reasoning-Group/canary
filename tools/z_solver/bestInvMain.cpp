#include<ctime>
#include<iostream>
#include<algorithm>
#include"csvDealer.h"
#include"parameterParser.h"
#include"bestInv/chcParser.h"
#include"bestInv/invTemplate.h"
#include"kInduction/kInductor.h"

using namespace std;
using namespace bestInv;

chcParser par;

double timestamp() {
    static double pre = 0;
    double now = double(clock()) / CLOCKS_PER_SEC;
    double ret = now - pre;
    pre = now;
    return ret;
}

int main(int argc, char** argv, char** env) {

#if WIN32

    transitionSystem ts = par.file_to_transys("D:/VSProjects/z_solver/data/nove/32lia/const_false-unreach-call1.sl_32bits_unsigned.sl_32bits_unsigned.smt2");

    UBVIntervalInvariant inv1(ts);
    UBVOctagonInvariant inv2(ts);
    ABVBitwiseInvariant inv3(ts), inv4(ts);

    timestamp();

    inv1.evaluate_to_best_Lift_Model_Bound_Heu(-1);
    for (int i = 0; i < inv1.l_val.size(); ++i) {
        cout << inv1.l_val[i] << ' ' << inv1.u_val[i] << endl;
    }
    cout << inv1.smt_count + inv1.omt_count << ' ' << timestamp() << endl;

    //inv2.evaluate_to_best_Decr(-1);
    //for (int i = 0; i < inv2.l_val.size(); ++i) {
    //    cout << inv2.l_val[i] << ' ' << inv2.u_val[i] << endl;
    //}
    //cout << inv2.smt_count + inv2.omt_count << ' ' << timestamp() << endl;

    inv3.evaluate_to_best_Fix_Bilateral(-1);
    cout << inv3.val_string() << endl;
    cout << inv3.smt_count + inv3.omt_count << ' ' << timestamp() << endl;

    inv4.evaluate_to_best_Guess(-1);
    cout << inv4.val_string() << endl;
    cout << inv4.smt_count + inv4.omt_count << ' ' << timestamp() << endl;

#else
    try {
        bestInv::parameterParser paras(argc, argv);
        csvDealer csv(paras.output);

        transitionSystem ts = par.file_to_transys(paras.file);

        double st, et;

        if (paras.domain == "octagon") {
            UBVOctagonInvariant inv(ts);
            if (paras.method == "omt") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_OMT(paras.timeout);
            }
            else if (paras.method == "bi") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Fix_Bilateral(paras.timeout);
            }
            else if (paras.method == "decr") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Decr(paras.timeout);
            }
            else if (paras.method == "optbin") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Lift_Model_Bound_Heu(paras.timeout);
            }
            et = double(clock()) / CLOCKS_PER_SEC;
            csv.write_append(csvLine(paras.file, paras.method, inv.bad_solve, inv.smt_count + inv.omt_count, paras.timeout < 0 ? et - st : std::min(et - st, paras.timeout), inv.l_val_string() + inv.u_val_string()));
        }
        else if (paras.domain == "interval") {
            UBVIntervalInvariant inv(ts);
            if (paras.method == "omt") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_OMT(paras.timeout);
            }
            else if (paras.method == "binlift") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Lift(paras.timeout);
            }
            else if (paras.method == "binlift+") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Lift_Model(paras.timeout);
            }
            else if (paras.method == "binlift++") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Lift_Model_Bound(paras.timeout);
            }
            else if (paras.method == "binlift-+") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Lift_Bound_Heu(paras.timeout);
            }
            else if (paras.method == "binlift-") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Lift_Bound(paras.timeout);
            }
            else if (paras.method == "decr") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Decr(paras.timeout);
            }
            else if (paras.method == "incr") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Fix_Single(paras.timeout);
            }
            else if (paras.method == "omtfix") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Fix_OMT(paras.timeout);
            }
            else if (paras.method == "lenbs") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Len_Single(paras.timeout);
            }
            else if (paras.method == "optbin") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Lift_Model_Bound_Heu(paras.timeout);
            }
            else if (paras.method == "binfix") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Fix_BS(paras.timeout);
            }
            else if (paras.method == "bi") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Fix_Bilateral(paras.timeout);
            }
            else {
                throw string("Unexpected method at main()!");
            }
            et = double(clock()) / CLOCKS_PER_SEC;
            csv.write_append(csvLine(paras.file, paras.method, inv.bad_solve, inv.smt_count + inv.omt_count, paras.timeout < 0 ? et - st : std::min(et - st, paras.timeout), inv.l_val_string() + inv.u_val_string()));
        }
        else if (paras.domain == "bitwise") {
            ABVBitwiseInvariant inv(ts);
            if (paras.method == "guess") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Guess(paras.timeout);
            }
            else if (paras.method == "bi") {
                st = double(clock()) / CLOCKS_PER_SEC;
                inv.evaluate_to_best_Fix_Bilateral(paras.timeout);
            }
            et = double(clock()) / CLOCKS_PER_SEC;
            csv.write_append(csvLine(paras.file, paras.method, inv.bad_solve, inv.smt_count + inv.omt_count, paras.timeout < 0 ? et - st : std::min(et - st, paras.timeout), inv.val_string()));
        }
    }
    catch(string str) {
        cerr << "Error: " + str << endl;
    }
#endif
    return 0;
}