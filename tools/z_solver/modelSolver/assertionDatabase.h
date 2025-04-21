#pragma once
#include<map>
#include<mutex>
#include<vector>
#include"generalExpr.h"

struct assertionDatabase {
	static int maxSize;
	static std::mutex lock;

	int size;
	int* depth;
	int* father[32];
	std::map<generalExpr, int>** child;
	generalExpr* node_expr;

	assertionDatabase();
	~assertionDatabase();

	static int getLCA(int id1, int id2);
	static int add_assertion(int cur, generalExpr expr);
};