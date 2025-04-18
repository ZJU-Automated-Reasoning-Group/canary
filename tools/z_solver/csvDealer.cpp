#include<fstream>
#include"csvDealer.h"

using namespace std;

bestInv::csvLine::csvLine(string filename, string method, int bad, int calls, double time, string info)
	:filename(filename), method(method), bad(bad), calls(calls), time(time), info(info) {
}

string bestInv::csvLine::to_string() {
	return filename + "," + method + "," + std::to_string(bad) + "," + std::to_string(time) + "," + std::to_string(calls) + "," + info;
}

bestInv::csvDealer::csvDealer(string filename) {
	ifstream in(filename);
	bool has_file = in.good();
	in.close();

	out.open(filename, ios::app);
	if (!has_file) {
		out << "File,Method,Bad Solve,Time Cost,Calls,Info" << endl;
	}
}

void kInduction::csvDealer::write_append(csvLine line) {
	out << line.to_string() << endl;
}

kInduction::csvLine::csvLine(std::string filename, std::string method, bool proved, bool result, int kreach, double time)
	:filename(filename), method(method), proved(proved), result(result), kreach(kreach), time(time) {
}

string kInduction::csvLine::to_string() {
	return filename + "," + method + "," + std::to_string(proved) + "," + std::to_string(result) + "," + std::to_string(kreach) + "," + std::to_string(time);
}

kInduction::csvDealer::csvDealer(string filename) {
	ifstream in(filename);
	bool has_file = in.good();
	in.close();

	out.open(filename, ios::app);
	if (!has_file) {
		out << "File,Method,Proved,Result,K_needed,Time Cost" << endl;
	}
}

void bestInv::csvDealer::write_append(csvLine line) {
	out << line.to_string() << endl;
}