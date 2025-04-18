#pragma once
#include<mutex>
#include"z3++.h"

struct generalExpr {

	std::shared_ptr<z3::expr> expr;

	static std::mutex& getLock();

	generalExpr();
	generalExpr(z3::expr z3expr);
	generalExpr(const generalExpr& gexpr);
	generalExpr& operator=(const generalExpr& gexpr);

	z3::expr expr_for_ctx(z3::context& ctx);
	bool is_true();
	generalExpr simplify();

	bool operator<(const generalExpr& rvalue) const;
	generalExpr operator+(const generalExpr& rvalue) const;
	generalExpr operator-(const generalExpr& rvalue) const;
	generalExpr operator&&(const generalExpr& rvalue) const;
	static generalExpr ule(const generalExpr& a, const generalExpr& b);
	static generalExpr ult(const generalExpr& a, const generalExpr& b);

};