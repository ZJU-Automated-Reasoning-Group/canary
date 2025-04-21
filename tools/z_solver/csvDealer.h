#pragma once
#include<string>
#include<vector>
#include<fstream>

namespace bestInv {
	struct csvLine {
		std::string filename;
		std::string method;
		int bad;
		int calls;
		double time;
		std::string info;

		csvLine(std::string filename, std::string method, int bad, int calls, double time, std::string info);
		std::string to_string();
	};

	struct csvDealer {
		std::ofstream out;

		csvDealer(std::string filename);
		void write_append(csvLine line);
	};
}

namespace kInduction {
	struct csvLine {
		std::string filename;
		std::string method;
		bool proved;
		bool result;
		int kreach;
		double time;

		csvLine(std::string filename, std::string method, bool proved, bool result, int kreach, double time);
		std::string to_string();
	};

	struct csvDealer {
		std::ofstream out;

		csvDealer(std::string filename);
		void write_append(csvLine line);
	};
}

namespace NumeralAnalyzer {
	struct csvLine {
		std::string filename;
		std::string tool;
		bool proved;
		bool result;
		double time;

		csvLine(std::string filename, std::string tool, bool proved, bool result, double time);
		std::string to_string();
	};

	struct csvDealer {
		std::ofstream out;

		csvDealer(std::string filename);
		void write_append(csvLine line);
	};
}