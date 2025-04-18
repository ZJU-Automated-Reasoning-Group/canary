#pragma once
#include"transitionSystem.h"

struct chcParser {
	z3::context& ctx;

	chcParser();
	~chcParser();

	transitionSystem file_to_transys(std::string filepath);

	transitionSystem string_to_transys(std::string chc_problem);
};