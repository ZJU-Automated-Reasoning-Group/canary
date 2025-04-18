#pragma once
#include<string>

namespace bestInv {
	/*
	parameters:
	  -F file : A CHC smt2 file
	  -O output : The csv file to be written
	  -T timeout : Timeout in seconds (-1 for no timeout and as default)
	  -D domain([octagon, interval, bitwise]) : The invariant domain
	  -M method([...]) : The bestInv algorithm to run
	*/
	struct parameterParser {
		std::string file;
		std::string output;
		double timeout;
		std::string domain;
		std::string method;

		parameterParser(int argc, char** argv);
		void printHelp();
	};
}

namespace kInduction {
	/*
	parameters:
	  -F file : A CHC smt2 file
	  -O output : The csv file to be written
	  -T timeout : Timeout in seconds (-1 for no timeout and as default)
	  -K K-limit : the maximum value of K (64 as default)
	  -D domain([octagon, interval, bitwise]) : The invariant domain ("none" for not using)
	  -M method([...]) : The bestInv algorithm to run ("none" for not using)
	*/
	struct parameterParser {
		std::string file;
		std::string output;
		double timeout;
		int klim;
		std::string domain;
		std::string method;

		parameterParser(int argc, char** argv);
		void printHelp();
	};
}