/*
This file contains utility functions for using Z3 in C++.
(Translated from my Python code by LLM (to be checked))

 * APIs included:
 * - absolute_value_bv: Computes absolute value of bit-vector expressions
 * - absolute_value_int: Computes absolute value of integer expressions
 * - get_expr_vars: Extracts all variables from an expression
 * - is_expr_var: Checks if an expression is a variable
 * - is_expr_val: Checks if an expression is a value
 * - is_term: Checks if an expression is a term
 * - is_atom: Checks if an expression is an atom
 * - is_pos_lit: Checks if an expression is a positive literal
 * - is_neg_lit: Checks if an expression is a negative literal
 * - is_lit: Checks if an expression is a literal
 * - negate: Negates a formula
 * - big_and: Creates a conjunction of expressions
 * - big_or: Creates a disjunction of expressions
 * - skolemize: Converts expression to Skolem normal form
 * - create_function_body_str: Creates function body strings for SMT-LIB2
 * - eval_predicates: Evaluates predicates in a model
 * - get_z3_logic: Gets the Z3 logic of a formula
 * - FormulaInfo: Class to analyze formula properties
 */


#ifndef Z3_UTILS_H
#define Z3_UTILS_H

#include <z3++.h>
#include <z3.h>
#include <vector>
#include <set>
#include <string>
#include <map>
#include <algorithm>
#include <iostream>
#include <cassert>

namespace z3utils {

/**
 * Computes the absolute value of a bit-vector expression
 * Based on: https://graphics.stanford.edu/~seander/bithacks.html#IntegerAbs
 */
z3::expr absolute_value_bv(const z3::expr& bv) {
    z3::context& ctx = bv.ctx();
    unsigned size = bv.get_sort().bv_size();
    z3::expr mask = bv.ctx().bv_val(0, size);  // Default to 0
    
    if (size > 0) {
        mask = z3::ashr(bv, size - 1);
    }
    
    return mask ^ (bv + mask);
}

/**
 * Computes the absolute value of an integer expression
 */
z3::expr absolute_value_int(const z3::expr& x) {
    z3::context& ctx = x.ctx();
    return z3::ite(x >= 0, x, -x);
}

/**
 * Gets all variables in an expression
 */
std::vector<z3::expr> get_expr_vars(const z3::expr& e) {
    try {
        std::set<z3::expr> syms;
        std::vector<z3::expr> stack;
        stack.push_back(e);

        while (!stack.empty()) {
            z3::expr current = stack.back();
            stack.pop_back();

            if (current.is_app()) {
                z3::func_decl decl = current.decl();
                if (current.num_args() == 0 && decl.decl_kind() == Z3_OP_UNINTERPRETED) {
                    syms.insert(current);
                } else {
                    for (unsigned i = 0; i < current.num_args(); ++i) {
                        stack.push_back(current.arg(i));
                    }
                }
            }
        }

        return std::vector<z3::expr>(syms.begin(), syms.end());
    } catch (const z3::exception& ex) {
        std::cerr << "Z3 exception: " << ex << std::endl;
        return std::vector<z3::expr>();
    }
}

/**
 * Gets all variables with optimization for shared subexpressions
 */
std::vector<z3::expr> get_expr_vars_v2(const z3::expr& e) {
    try {
        std::set<z3::expr> syms;
        std::set<unsigned> visited;
        std::vector<z3::expr> stack;
        stack.push_back(e);

        while (!stack.empty()) {
            z3::expr current = stack.back();
            stack.pop_back();
            
            unsigned id = Z3_get_ast_id(current.ctx(), current);
            
            if (visited.find(id) != visited.end()) {
                continue;
            }
            
            visited.insert(id);
            
            if (current.is_app()) {
                z3::func_decl decl = current.decl();
                if (current.num_args() == 0 && decl.decl_kind() == Z3_OP_UNINTERPRETED) {
                    syms.insert(current);
                } else {
                    for (unsigned i = 0; i < current.num_args(); ++i) {
                        stack.push_back(current.arg(i));
                    }
                }
            }
        }

        return std::vector<z3::expr>(syms.begin(), syms.end());
    } catch (const z3::exception& ex) {
        std::cerr << "Z3 exception: " << ex << std::endl;
        return std::vector<z3::expr>();
    }
}

/**
 * Gets all variables in an expression (wrapper function)
 */
std::vector<z3::expr> get_variables(const z3::expr& e) {
    return get_expr_vars(e);
}

/**
 * Gets all atomic predicates in a formula
 */
std::set<z3::expr> get_atoms(const z3::expr& expr) {
    std::set<z3::expr> s;
    
    std::function<void(const z3::expr&)> get_preds = [&](const z3::expr& exp) {
        // Skip if already processed
        if (s.find(exp) != s.end()) {
            return;
        }
        
        if (exp.is_not()) {
            s.insert(exp);
        }
        
        if (exp.is_and() || exp.is_or()) {
            for (unsigned i = 0; i < exp.num_args(); ++i) {
                get_preds(exp.arg(i));
            }
            return;
        }
        
        assert(exp.is_bool());
        s.insert(exp);
    };
    
    // Convert to NNF and look for predicates
    z3::tactic nnf_tactic(expr.ctx(), "nnf");
    z3::goal g(expr.ctx());
    g.add(expr);
    z3::apply_result result = nnf_tactic(g);
    
    if (result.size() > 0) {
        z3::expr nnf_expr = result[0].as_expr();
        get_preds(nnf_expr);
    }
    
    return s;
}

/**
 * Converts expression to SMT-LIB2 string
 */
std::string to_smtlib2(const z3::expr& expr) {
    z3::solver s(expr.ctx());
    s.add(expr);
    return s.to_smt2();
}

/**
 * Checks if an expression is a function symbol
 */
bool is_function_symbol(const z3::expr& s) {
    if (!s.is_app()) {
        return false;
    }
    
    if (s.is_const()) {
        return false;
    }
    
    z3::func_decl func = s.decl();
    
    if (func.range().is_bool()) {
        // Predicate symbol
        return false;
    }
    
    std::string name = func.name().str();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    
    if (name == "if") {
        return false;
    }
    
    return true;
}

/**
 * Finds all function symbols in an expression
 */
std::set<z3::func_decl> get_function_symbols(const z3::expr& s) {
    std::set<z3::func_decl> fsymbols;
    
    if (is_function_symbol(s)) {
        fsymbols.insert(s.decl());
    }
    
    for (unsigned i = 0; i < s.num_args(); ++i) {
        std::set<z3::func_decl> child_symbols = get_function_symbols(s.arg(i));
        fsymbols.insert(child_symbols.begin(), child_symbols.end());
    }
    
    return fsymbols;
}

/**
 * Converts expression to Skolem normal form
 */
z3::expr skolemize(const z3::expr& exp) {
    z3::goal g(exp.ctx());
    g.add(exp);
    z3::tactic t(exp.ctx(), "snf");
    z3::apply_result res = t(g);
    
    if (res.size() > 0) {
        return res[0].as_expr();
    }
    
    return exp; // Return original if conversion fails
}

/**
 * Creates a conjunction of expressions
 */
z3::expr big_and(const std::vector<z3::expr>& exprs) {
    if (exprs.empty()) {
        return exprs[0].ctx().bool_val(true);
    }
    
    if (exprs.size() == 1) {
        return exprs[0];
    }
    
    z3::expr_vector ev(exprs[0].ctx());
    for (const auto& e : exprs) {
        ev.push_back(e);
    }
    
    return z3::mk_and(ev);
}

/**
 * Creates a disjunction of expressions
 */
z3::expr big_or(const std::vector<z3::expr>& exprs) {
    if (exprs.empty()) {
        return exprs[0].ctx().bool_val(false);
    }
    
    if (exprs.size() == 1) {
        return exprs[0];
    }
    
    z3::expr_vector ev(exprs[0].ctx());
    for (const auto& e : exprs) {
        ev.push_back(e);
    }
    
    return z3::mk_or(ev);
}

/**
 * Negates a formula
 */
z3::expr negate(const z3::expr& f) {
    if (f.is_not()) {
        return f.arg(0);
    } else {
        return !f;
    }
}


/**
 * Checks if an expression is a variable
 */
bool is_expr_var(const z3::expr& a) {
    return a.is_const() && a.decl().decl_kind() == Z3_OP_UNINTERPRETED;
}

/**
 * Checks if an expression is a value
 */
bool is_expr_val(const z3::expr& a) {
    return a.is_const() && a.decl().decl_kind() != Z3_OP_UNINTERPRETED;
}

/**
 * Checks if an expression is a term
 */
bool is_term(const z3::expr& a) {
    if (!a.is_expr()) {
        return false;
    }
    
    if (a.is_const()) {
        return true;
    } else {
        if (!a.is_bool()) {
            for (unsigned i = 0; i < a.num_args(); ++i) {
                if (!is_term(a.arg(i))) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
}

/**
 * List of connective operations
 */
const std::vector<Z3_decl_kind> CONNECTIVE_OPS = {
    Z3_OP_NOT, Z3_OP_AND, Z3_OP_OR, Z3_OP_IMPLIES,
    Z3_OP_IFF, Z3_OP_ITE, Z3_OP_XOR
};

/**
 * Checks if an expression is an atom
 */
bool is_atom(const z3::expr& a) {
    if (!a.is_bool()) {
        return false;
    }
    
    if (is_expr_val(a)) {
        return false;
    }
    
    if (is_expr_var(a)) {
        return true;
    }
    
    if (a.is_app()) {
        Z3_decl_kind kind = a.decl().decl_kind();
        auto it = std::find(CONNECTIVE_OPS.begin(), CONNECTIVE_OPS.end(), kind);
        if (it == CONNECTIVE_OPS.end()) {
            for (unsigned i = 0; i < a.num_args(); ++i) {
                if (!is_term(a.arg(i))) {
                    return false;
                }
            }
            return true;
        }
    }
    
    return false;
}

/**
 * Checks if an expression is a positive literal
 */
bool is_pos_lit(const z3::expr& a) {
    return is_atom(a);
}

/**
 * Checks if an expression is a negative literal
 */
bool is_neg_lit(const z3::expr& a) {
    return a.is_not() && is_pos_lit(a.arg(0));
}

/**
 * Checks if an expression is a literal
 */
bool is_lit(const z3::expr& a) {
    return is_pos_lit(a) || is_neg_lit(a);
}

/**
 * Creates function body strings for SMT-LIB2
 */
std::vector<std::string> create_function_body_str(
    const std::string& funcname,
    const std::vector<z3::expr>& varlist,
    const z3::expr& body) 
{
    std::vector<std::string> res;
    std::string target = "(define-fun " + funcname + " (";
    
    for (size_t i = 0; i < varlist.size(); ++i) {
        target += "(" + varlist[i].to_string() + " " + 
                  varlist[i].get_sort().to_string() + ") ";
    }
    
    target += ") Bool " + body.to_string() + ")";
    res.push_back(target);
    
    for (const auto& var : varlist) {
        res.push_back("(declare-const " + var.to_string() + " " + 
                      var.get_sort().to_string() + ")");
    }
    
    return res;
}

/**
 * Evaluates predicates in a model
 */
std::vector<z3::expr> eval_predicates(const z3::model& m, const std::vector<z3::expr>& predicates) {
    std::vector<z3::expr> res;
    
    for (const auto& p : predicates) {
        z3::expr eval_result = m.eval(p);
        
        if (Z3_get_bool_value(p.ctx(), eval_result) == Z3_L_TRUE) {
            res.push_back(p);
        } else if (Z3_get_bool_value(p.ctx(), eval_result) == Z3_L_FALSE) {
            res.push_back(negate(p));
        }
    }
    
    return res;
}


/**
 * Class to analyze formula properties
 */
class FormulaInfo {
public:
    FormulaInfo(const z3::expr& fml) : formula(fml) {
        has_quantifier_val = check_has_quantifier();
        logic_val = determine_logic();
    }
    
    bool has_quantifier() const {
        return has_quantifier_val;
    }
    
    bool logic_has_bv() const {
        return logic_val.find("BV") != std::string::npos;
    }
    
    std::string get_logic() const {
        return logic_val;
    }
    
private:
    z3::expr formula;
    bool has_quantifier_val;
    std::string logic_val;
    
    bool apply_probe(const char* name) {
        z3::goal g(formula.ctx());
        g.add(formula);
        z3::probe p(formula.ctx(), name);
        return p(g) != 0;
    }
    
    bool check_has_quantifier() {
        return apply_probe("has-quantifiers");
    }
    
    std::string determine_logic() {
        try {
            if (!has_quantifier_val) {
                if (apply_probe("is-propositional")) {
                    return "QF_UF";
                } else if (apply_probe("is-qfbv")) {
                    return "QF_BV";
                } else if (apply_probe("is-qfaufbv")) {
                    return "QF_AUFBV";
                } else if (apply_probe("is-qflia")) {
                    return "QF_LIA";
                } else if (apply_probe("is-qflra")) {
                    return "QF_LRA";
                } else if (apply_probe("is-qflira")) {
                    return "QF_LIRA";
                } else if (apply_probe("is-qfnia")) {
                    return "QF_NIA";
                } else if (apply_probe("is-qfnra")) {
                    return "QF_NRA";
                } else if (apply_probe("is-qfufnra")) {
                    return "QF_UFNRA";
                } else {
                    return "ALL";
                }
            } else {
                if (apply_probe("is-lia")) {
                    return "LIA";
                } else if (apply_probe("is-lra")) {
                    return "LRA";
                } else if (apply_probe("is-lira")) {
                    return "LIRA";
                } else if (apply_probe("is-nia")) {
                    return "NIA";
                } else if (apply_probe("is-nra")) {
                    return "NRA";
                } else if (apply_probe("is-nira")) {
                    return "NIRA";
                } else {
                    return "ALL";
                }
            }
        } catch (const z3::exception& ex) {
            std::cerr << "Z3 exception: " << ex << std::endl;
            return "ALL";
        }
    }
};

/**
 * Gets the Z3 logic of a formula
 */
std::string get_z3_logic(const z3::expr& fml) {
    FormulaInfo fml_info(fml);
    return fml_info.get_logic();
}


} // namespace z3utils

#endif // Z3_UTILS_H