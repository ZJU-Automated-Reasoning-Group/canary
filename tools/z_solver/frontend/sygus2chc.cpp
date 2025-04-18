/**
 * @file sygus2chc.cpp
 * @brief Command-line tool to convert SyGuS specifications to CHC format
 * 
 * This tool takes a SyGuS file as input and converts it to CHC (Constrained Horn Clauses)
 * format, which can then be used by verification tools.
 */

#include "SyGuSParser.h"
#include <iostream>
#include <string>
#include <filesystem>

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " <input_sygus_file> [output_chc_file]" << std::endl;
    std::cerr << "If output_chc_file is not specified, it will be derived from the input file name." << std::endl;
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc < 2 || argc > 3) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile;

    // If output file is not specified, derive it from input file
    if (argc == 2) {
        std::filesystem::path inputPath(inputFile);
        outputFile = inputPath.stem().string() + ".smt2";
    } else {
        outputFile = argv[2];
    }

    try {
        std::cout << "Converting SyGuS file '" << inputFile << "' to CHC format..." << std::endl;
        SyGuSParser::sygus_to_chc(inputFile, outputFile);
        std::cout << "Conversion complete. Output written to '" << outputFile << "'" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error during conversion: " << e.what() << std::endl;
        return 1;
    }
} 