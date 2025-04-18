#include"generalExpr.h"
#include"assertionDatabase.h"

using namespace std;

extern z3::context _BASE_CTX;
extern assertionDatabase _BASE_ENV;

mutex& generalExpr::getLock() {
	static mutex lock;
	return lock;
}

generalExpr::generalExpr()
	:expr(nullptr) {
	getLock().lock();
	expr = make_shared<z3::expr>(_BASE_CTX);
	getLock().unlock();
}

generalExpr::generalExpr(z3::expr z3expr)
	:expr(nullptr) {
	getLock().lock();
	expr = make_shared<z3::expr>(z3::to_expr(_BASE_CTX, Z3_translate(z3expr.ctx(), z3expr, _BASE_CTX)));
	getLock().unlock();
}

generalExpr::generalExpr(const generalExpr& gexpr)
	: expr(nullptr) {
	 getLock().lock();
	 expr = gexpr.expr;
	 getLock().unlock();
}

generalExpr& generalExpr::operator=(const generalExpr& gexpr) {
	 getLock().lock();
	 expr = gexpr.expr;
	 getLock().unlock();
	 return *this;
}

z3::expr generalExpr::expr_for_ctx(z3::context& ctx) {
	getLock().lock();
	z3::expr ret = z3::to_expr(ctx, Z3_translate(_BASE_CTX, *expr, ctx));
	getLock().unlock();
	return ret;
}

bool generalExpr::operator<(const generalExpr& rvalue) const {
	getLock().lock();
	bool ret = expr->to_string() < rvalue.expr->to_string();
	getLock().unlock();
	return ret;
}

generalExpr generalExpr::operator+(const generalExpr& rvalue) const {
	getLock().lock();
	generalExpr ret = *expr + *rvalue.expr;
	getLock().unlock();
	return ret;
}

generalExpr generalExpr::operator-(const generalExpr& rvalue) const {
	getLock().lock();
	generalExpr ret = *expr - *rvalue.expr;
	getLock().unlock();
	return ret;
}

generalExpr generalExpr::operator&&(const generalExpr& rvalue) const {
	getLock().lock();
	generalExpr ret = *expr && *rvalue.expr;
	getLock().unlock();
	return ret;
}

bool generalExpr::is_true() {
	getLock().lock();
	bool ret = expr->is_true();
	getLock().unlock();
	return ret;
}

generalExpr generalExpr::simplify() {
	getLock().lock();
	generalExpr ret = expr->simplify();
	getLock().unlock();
	return ret;
}

generalExpr generalExpr::ule(const generalExpr& a, const generalExpr& b) {
	getLock().lock();
	generalExpr ret = z3::ule(*a.expr, *b.expr);
	getLock().unlock();
	return ret;
}

generalExpr generalExpr::ult(const generalExpr& a, const generalExpr& b) {
	getLock().lock();
	generalExpr ret = z3::ult(*a.expr, *b.expr);
	getLock().unlock();
	return ret;
}
