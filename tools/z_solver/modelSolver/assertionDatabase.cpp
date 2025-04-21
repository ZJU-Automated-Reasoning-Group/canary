#include"assertionDatabase.h"

using namespace std;

int assertionDatabase::maxSize = 1e5;

z3::context _BASE_CTX;
assertionDatabase _BASE_ENV;

mutex assertionDatabase::lock{};

assertionDatabase::assertionDatabase()
	: size(1), depth(new int[maxSize]), father(), child(new std::map<generalExpr, int>*[maxSize]), node_expr(new generalExpr[maxSize]) {
	depth[0] = -1;
	depth[1] = 0;
	for (int i = 0; i < 32; ++i) {
		father[i] = new int[maxSize];
		father[i][0] = father[i][1] = 0;
	}
	child[1] = new map<generalExpr, int>();
}

assertionDatabase::~assertionDatabase() {
	delete[] depth;
	for (int i = 0; i < 32; ++i) {
		delete[] father[i];
	}
	for (auto i = 1; i <= size; ++i) {
		delete child[i];
	}
	delete[] child;
	delete[] node_expr;
}

int assertionDatabase::add_assertion(int cur, generalExpr expr) {
	lock.lock();
	int nova = _BASE_ENV.size++;
	_BASE_ENV.child[cur]->insert(make_pair(expr, nova));
	_BASE_ENV.depth[nova] = _BASE_ENV.depth[cur] + 1;
	_BASE_ENV.father[0][nova] = cur;
	for (int i = 1; i < 32; ++i) { _BASE_ENV.father[i][nova] = _BASE_ENV.father[i - 1][_BASE_ENV.father[i - 1][nova]]; }
	_BASE_ENV.child[nova] = new map<generalExpr, int>();
	_BASE_ENV.node_expr[nova] = expr;
	lock.unlock();
	return nova;
}

int assertionDatabase::getLCA(int id1, int id2) {
	int a = id1, b = id2;
	if (_BASE_ENV.depth[a] < _BASE_ENV.depth[b]) swap(a, b);
	for(int i = 31; i >= 0; --i) {
		if (_BASE_ENV.father[i][a] != -1 && _BASE_ENV.depth[_BASE_ENV.father[i][a]] >= _BASE_ENV.depth[b]) {
			a = _BASE_ENV.father[i][a];
		}
	}
	if (a == b) return a;
	for (int i = 31; i >= 0; --i) {
		if (_BASE_ENV.father[i][a] != _BASE_ENV.father[i][b]) {
			a = _BASE_ENV.father[i][a];
			b = _BASE_ENV.father[i][b];
		}
	}
	return _BASE_ENV.father[0][a];
}