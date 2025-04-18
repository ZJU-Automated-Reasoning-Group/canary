#include"parameterParser.h"
#include<iostream>

using namespace std;

bestInv::parameterParser::parameterParser(int argc, char** argv)
	: timeout(-1) {
	// Check for -h flag first before processing other parameters
	for (int i = 1; i < argc; ++i) {
		if (std::string(argv[i]) == "-h") {
			printHelp();
			// printHelp() calls exit(0), so we won't reach here
		}
	}
	
	for (int i = 1; i < argc; ++i) {
		string para = argv[i];
		if (para == "-F" && ++i < argc) {
			file = argv[i];
		}
		else if (para == "-O" && ++i < argc) {
			output = argv[i];
		}
		else if (para == "-T" && ++i < argc) {
			timeout = stod(argv[i]);
		}
		else if (para == "-D" && ++i < argc) {
			domain = argv[i];
		}
		else if (para == "-M" && ++i < argc) {
			method = argv[i];
		}
		else if (para == "-h") {
			// Already handled above
		}
		else {
			throw string("Unrecognized parameter at parameterParser()!");
		}
	}
	
	if (file == "" || output == "" || domain == "" || method == "") {
		throw string("Parameter missing at parameterParser()!");
	}
}

void bestInv::parameterParser::printHelp() {
	cout << "Tool Usage:" << endl;
	cout << "    bestInv -F <input_file> -O <output_csv> -D <domain> -M <method> [-T <timeout>]" << endl;
	cout << endl;
	cout << "Parameters:" << endl;
	cout << "  -F file    : Input CHC SMT2 file" << endl;
	cout << "  -O output  : CSV file to write results" << endl;
	cout << "  -D domain  : Invariant domain (bestInv: interval/octagon/bitwise, kInductor: interval/none)" << endl;
	cout << "  -M method  : Algorithm to use, see following(can be none for kInductor)" << endl;
	cout << "  -T timeout : Synthesis timeout in seconds (-1 for no timeout, default)" << endl;
	cout << "  -h         : Display this help information" << endl;
	cout << endl;
	cout << "Available methods:" << endl;
	cout << "  For interval domain: binlift, decr, incr, omtfix, lenbs, optbin, binfix, bi" << endl;
	cout << "  For octagon domain: bi, decr, optbin" << endl;
	cout << "  For known-bits domain: bi, guess" << endl;
	cout << endl;
	cout << "Example:" << endl;
	cout << "  bestInv -F data/example.smt2 -O results.csv -D interval -M optbin" << endl;
	exit(0);
}

kInduction::parameterParser::parameterParser(int argc, char** argv)
	: timeout(-1), klim(64) {
	// Check for -h flag first before processing other parameters
	for (int i = 1; i < argc; ++i) {
		if (std::string(argv[i]) == "-h") {
			printHelp();
			// printHelp() calls exit(0), so we won't reach here
		}
	}
	
	for (int i = 1; i < argc; ++i) {
		string para = argv[i];
		if (para == "-F" && ++i < argc) {
			file = argv[i];
		}
		else if (para == "-O" && ++i < argc) {
			output = argv[i];
		}
		else if (para == "-T" && ++i < argc) {
			timeout = stod(argv[i]);
		}
		else if (para == "-K" && ++i < argc) {
			klim = stoi(argv[i]);
		}
		else if (para == "-D" && ++i < argc) {
			domain = argv[i];
		}
		else if (para == "-M" && ++i < argc) {
			method = argv[i];
		}
		else if (para == "-h") {
			// Already handled above
		}
		else {
			throw string("Unrecognized parameter at parameterParser()!");
		}
	}
	
	if (file == "" || output == "") {
		throw string("Parameter missing at parameterParser()!");
	}
}

void kInduction::parameterParser::printHelp() {
	cout << "Tool Usage:" << endl;
	cout << "    kInductor -F <input_file> -O <output_csv> -D <domain> -M <method> [-K <k_limit>] [-T <timeout>]" << endl;
	cout << endl;
	cout << "Parameters:" << endl;
	cout << "  -F file    : Input CHC SMT2 file" << endl;
	cout << "  -O output  : CSV file to write results" << endl;
	cout << "  -D domain  : Invariant domain (bestInv: interval/octagon/bitwise, kInductor: interval/none)" << endl;
	cout << "  -M method  : Algorithm to use, see following(can be none for kInductor)" << endl;
	cout << "  -T timeout : Synthesis timeout in seconds (-1 for no timeout, default)" << endl;
	cout << "  -K k-limit : Maximum value of K for k-induction (default: 64)" << endl;
	cout << "  -h         : Display this help information" << endl;
	cout << endl;
	cout << "Available methods:" << endl;
	cout << "  For kInductor with interval: decr, optbin, bi" << endl;
	cout << endl;
	cout << "Example:" << endl;
	cout << "  kInductor -F data/example.smt2 -O results.csv -D none -M none -K 10" << endl;
	exit(0);
}
