#pragma once
#include"transitionSystem.h"

struct invariant {
	z3::context& ctx;
	transitionSystem trans;

	z3::expr inv_cons;
	z3::expr inv_bar_cons;
	z3::expr pre_cons;
	z3::expr trans_cons;
	z3::expr post_cons;

	int smt_count;
	int omt_count;
	int bad_solve; // 1 for unknown, 2 for timeout
	clock_t time_start;
	clock_t time_out;

	invariant(transitionSystem trans);

	void set_timer(double timeout);
	bool timeout_check();
};

struct UBVIntervalInvariant : public invariant {
	int size;
	std::vector<z3::expr> var;
	std::vector<z3::expr> var_bar;
	std::vector<int> size_var;

	std::vector<z3::expr> l_var;
	std::vector<z3::expr> u_var;

	std::vector<z3::expr> l_val;
	std::vector<z3::expr> u_val;

	UBVIntervalInvariant(transitionSystem trans);

	std::string l_val_string();
	std::string u_val_string();

	void initialize_inv_minmax();

	z3::expr get_current_inv();
	z3::expr get_current_inv_bar();

	bool check_single_base(int& entry, z3::expr& l, z3::expr& u, z3::solver& sol);
	bool check_single_model(int& entry, z3::expr& l, z3::expr& u, z3::solver& sol);
	bool check_single_bound(int& entry, z3::expr& l, z3::expr& u, z3::expr& lb, z3::expr& ub, z3::solver& sol);
	bool check_single_model_bound(int& entry, z3::expr& l, z3::expr& u, z3::expr& lb, z3::expr& ub, z3::solver& sol);
	bool check_single_len(int& entry, z3::expr& len, z3::solver& sol);
	bool check_total_len(z3::expr& cumu, z3::expr& len, z3::solver& sol);
	bool search_for_minmax(z3::solver& sol);
	bool search_for_minmax_bar(z3::solver& sol);
	bool bilateral_abstraction(z3::solver& sol, std::vector<z3::expr>& l_lower, std::vector<z3::expr>& u_lower, std::vector<z3::expr>& l_new, std::vector<z3::expr>& u_new);
	bool bilateral_abstraction_bar(z3::solver& sol, std::vector<z3::expr>& l_lower, std::vector<z3::expr>& u_lower, std::vector<z3::expr>& l_new, std::vector<z3::expr>& u_new);
	bool abstract_sequence(std::vector<z3::expr>& l_lower, std::vector<z3::expr>& l_upper, std::vector<z3::expr>& u_lower, std::vector<z3::expr>& u_upper, std::vector<z3::expr>& l_new, std::vector<z3::expr>& u_new);

	void evaluate_to_best_OMT(double timeout); //Timeout may not fit

	void evaluate_to_best_Lift(double timeout);
	void evaluate_to_best_Lift_Model(double timeout);
	void evaluate_to_best_Lift_Bound(double timeout);
	void evaluate_to_best_Lift_Bound_Heu(double timeout);
	void evaluate_to_best_Lift_Bound_FullHeu(double timeout);
	void evaluate_to_best_Lift_Model_Bound(double timeout);
	void evaluate_to_best_Lift_Model_Bound_Heu(double timeout);
	void evaluate_to_best_Lift_Model_Bound_FullHeu(double timeout);

	void evaluate_to_best_Lift_Model_Bound_Heu_Parallel(double timeout); //May need to rewrite by array (not vector), and need debug

	void evaluate_to_best_Decr(double timeout);
	void evaluate_to_best_Len_Single(double timeout);
	void evaluate_to_best_Len_Total(double timeout);

	void evaluate_to_best_Fix_Single(double timeout);
	void evaluate_to_best_Fix_OMT(double timeout);
	void evaluate_to_best_Fix_BS(double timeout); //Can optimize?
	void evaluate_to_best_Fix_Bilateral(double timeout);
};

struct UBVOctagonInvariant : public invariant {
	int size;
	std::vector<z3::expr> var;
	std::vector<z3::expr> var_bar;
	std::vector<int> size_var;

	std::vector<z3::expr> l_var;
	std::vector<z3::expr> u_var;

	std::vector<z3::expr> l_val;
	std::vector<z3::expr> u_val;

	UBVOctagonInvariant(transitionSystem trans);

	std::string l_val_string();
	std::string u_val_string();

	void initialize_inv_minmax();

	z3::expr get_current_inv();
	z3::expr get_current_inv_bar();

	bool check_single_model_bound(int& entry, z3::expr& l, z3::expr& u, z3::expr& lb, z3::expr& ub, z3::solver& sol);
	bool search_for_minmax(z3::solver& sol);
	bool search_for_minmax_bar(z3::solver& sol);
	bool bilateral_abstraction(z3::solver& sol, std::vector<z3::expr>& l_lower, std::vector<z3::expr>& u_lower, std::vector<z3::expr>& l_new, std::vector<z3::expr>& u_new);
	bool bilateral_abstraction_bar(z3::solver& sol, std::vector<z3::expr>& l_lower, std::vector<z3::expr>& u_lower, std::vector<z3::expr>& l_new, std::vector<z3::expr>& u_new);
	bool abstract_sequence(std::vector<z3::expr>& l_lower, std::vector<z3::expr>& l_upper, std::vector<z3::expr>& u_lower, std::vector<z3::expr>& u_upper, std::vector<z3::expr>& l_new, std::vector<z3::expr>& u_new);

	void evaluate_to_best_OMT(double timeout);
	void evaluate_to_best_Lift_Model_Bound_Heu(double timeout);
	void evaluate_to_best_Decr(double timeout);
	void evaluate_to_best_Fix_BS(double timeout); //Can optimize?
	void evaluate_to_best_Fix_Bilateral(double timeout);
};

struct ABVBitwiseInvariant : public invariant {
	int size;
	std::vector<int> belong;
	std::vector<int> pstion;
	std::vector<int> status;
	std::vector<z3::expr> vari;
	std::vector<z3::expr> varz;

	ABVBitwiseInvariant(transitionSystem trans);

	std::string val_string();

	z3::expr get_current_inv();
	z3::expr get_current_inv_bar();
	bool bilateral_abstraction(z3::solver& sol);
	bool bilateral_abstraction_bar(z3::solver& sol);

	void evaluate_to_best_Guess(double timeout);
	void evaluate_to_best_Fix_Bilateral(double timeout);
};