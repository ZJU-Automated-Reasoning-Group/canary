/*
cParser.h

This file contains the declaration of the cParser class.
The cParser class is used to parse a C program and convert it to a CHC (Constraint Horn Clauses).

*/

#pragma once
#include<string>

struct cParser {
	static void c_to_chc(std::string file_in, std::string file_out);
	
	// Helper methods for translating C code to SMT2 format
	static std::string translateCondition(const std::string& condition);
	static std::string translateExpression(const std::string& expr);
};