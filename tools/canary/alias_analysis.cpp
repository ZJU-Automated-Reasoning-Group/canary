#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include <cstdlib> // For getenv
#include <filesystem> // For path handling
#include <unistd.h> // For getcwd

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/ValueMap.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/TypeBasedAliasAnalysis.h>
#include <llvm/Analysis/ScopedNoAliasAA.h>
#include <llvm/Analysis/MemoryLocation.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IRReader/IRReader.h>

#include "Support/RecursiveTimer.h"
#include "DyckAA/DyckAliasAnalysis.h"
#include "CFLAA/CFLAndersAliasAnalysis.h"
#include "CFLAA/CFLSteensAliasAnalysis.h"

using namespace llvm;

cl::opt<std::string> BenchmarkPath("benchmark",
                                   cl::desc("Path to the LLVM bitcode file (~ will be expanded to your home directory)"),
                                   cl::init("benchmarks/spec2006/401.bzip2.bc"));

cl::opt<bool> Verbose("verbose",
                      cl::desc("Print verbose output about analysis"),
                      cl::init(false));

// Function to expand tilde in paths
std::string expandTilde(const std::string &path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }
    
    const char *homeDir = getenv("HOME");
    if (!homeDir) {
        std::cerr << "Warning: Could not get HOME environment variable to expand ~ in path\n";
        return path;
    }
    
    return std::string(homeDir) + path.substr(1);
}

// Function to check if a file exists
bool fileExists(const std::string &path) {
    std::ifstream file(path);
    return file.good();
}

// Function to resolve to absolute path if needed
std::string resolveToAbsolutePath(const std::string &path) {
    if (path.empty()) {
        return path;
    }
    
    // First expand tilde if present
    std::string expandedPath = expandTilde(path);
    
    // Then convert to absolute path if it's not already
    if (expandedPath[0] != '/') {
        char currentDir[4096];
        if (getcwd(currentDir, sizeof(currentDir)) != nullptr) {
            return std::string(currentDir) + "/" + expandedPath;
        } else {
            std::cerr << "Warning: Could not get current directory to resolve relative path\n";
        }
    }
    
    return expandedPath;
}

class AliasPercentageCalculator {
public:
    using AliasPair = std::pair<Value*, Value*>;
    using AliasResult = llvm::AliasResult;

    struct AnalysisResult {
        double aliasPercentage;
        double analysisTime;
        unsigned totalPairs;
        unsigned aliasedPairs;
    };

    std::vector<AliasPair> collectPointerPairs(Module &M) {
        std::vector<AliasPair> pairs;
        
        for (auto &F : M) {
            if (F.isDeclaration()) continue;
            
            std::vector<Value*> pointers;
            
            // Collect all pointer-typed values
            for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
                if (I->getType()->isPointerTy()) {
                    pointers.push_back(&*I);
                }
                
                // Also add pointer operands
                for (Use &U : I->operands()) {
                    if (U->getType()->isPointerTy()) {
                        pointers.push_back(U);
                    }
                }
            }
            
            // Create all possible pairs
            for (size_t i = 0; i < pointers.size(); ++i) {
                for (size_t j = i + 1; j < pointers.size(); ++j) {
                    pairs.emplace_back(pointers[i], pointers[j]);
                }
            }
        }
        
        return pairs;
    }

    AnalysisResult runDyckAA(Module &M, const std::vector<AliasPair> &pairs) {
        AnalysisResult result;
        result.totalPairs = pairs.size();
        result.aliasedPairs = 0;

        // Setup timer for measuring analysis time
        RecursiveTimer timer("DyckAA Analysis");
        auto startTime = std::chrono::steady_clock::now();
        
        // Create an instance of DyckAliasAnalysis
        DyckAliasAnalysis DAA;
        DAA.runOnModule(M);
        
        // Check each pair with DyckAA
        for (const auto &pair : pairs) {
            // Check alias relationship using mayAlias
            if (DAA.mayAlias(pair.first, pair.second)) {
                result.aliasedPairs++;
            }
        }
        
        // Calculate percentage and analysis time
        result.aliasPercentage = static_cast<double>(result.aliasedPairs) / result.totalPairs * 100.0;
        auto endTime = std::chrono::steady_clock::now();
        result.analysisTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() / 1000.0;
        std::cout << "DyckAA Stats Finished\n";
        
        return result;
    }

    AnalysisResult runCFLAndersAA(Module &M, const std::vector<AliasPair> &pairs) {
        AnalysisResult result;
        result.totalPairs = pairs.size();
        result.aliasedPairs = 0;

        // Setup timer for measuring analysis time
        auto startTime = std::chrono::steady_clock::now();
        
        // Create target library info
        TargetLibraryInfoImpl TLII;
        TargetLibraryInfo TLI(TLII); // Create a single instance
        auto getTargetLibraryInfo = [&TLI](Function &F) -> const TargetLibraryInfo & {
            return TLI; // Return reference to the existing object, not a temporary
        };
        
        // Create an instance of CFLAndersAAResult
        CFLAndersAAResult CFLAnders(getTargetLibraryInfo);
        
        // Check each pair
        for (const auto &pair : pairs) {
            // Create memory locations
            MemoryLocation LocA(pair.first, LocationSize::precise(8));
            MemoryLocation LocB(pair.second, LocationSize::precise(8));
            
            // Use query method directly which doesn't require AAQueryInfo
            if (CFLAnders.query(LocA, LocB) != llvm::AliasResult::NoAlias) {
                result.aliasedPairs++;
            }
        }
        
        // Calculate percentage and analysis time
        result.aliasPercentage = static_cast<double>(result.aliasedPairs) / result.totalPairs * 100.0;
        auto endTime = std::chrono::steady_clock::now();
        result.analysisTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() / 1000.0;
        
        return result;
    }

    AnalysisResult runCFLSteensAA(Module &M, const std::vector<AliasPair> &pairs) {
        AnalysisResult result;
        result.totalPairs = pairs.size();
        result.aliasedPairs = 0;

        // Setup timer for measuring analysis time
        auto startTime = std::chrono::steady_clock::now();
        
        // Create target library info
        TargetLibraryInfoImpl TLII;
        TargetLibraryInfo TLI(TLII); // Create a single instance
        auto getTargetLibraryInfo = [&TLI](Function &F) -> const TargetLibraryInfo & {
            return TLI; // Return reference to the existing object, not a temporary
        };
        
        // Create an instance of CFLSteensAAResult
        CFLSteensAAResult CFLSteens(getTargetLibraryInfo);
        
        // Check each pair
        for (const auto &pair : pairs) {
            // Create memory locations
            MemoryLocation LocA(pair.first, LocationSize::precise(8));
            MemoryLocation LocB(pair.second, LocationSize::precise(8));
            
            // Use query method directly which doesn't require AAQueryInfo
            if (CFLSteens.query(LocA, LocB) != llvm::AliasResult::NoAlias) {
                result.aliasedPairs++;
            }
        }
        
        // Calculate percentage and analysis time
        result.aliasPercentage = static_cast<double>(result.aliasedPairs) / result.totalPairs * 100.0;
        auto endTime = std::chrono::steady_clock::now();
        result.analysisTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() / 1000.0;
        
        return result;
    }

    void printComparison(const AnalysisResult &dyckResult, 
                         const AnalysisResult &andersResult, 
                         const AnalysisResult &steensResult) {
        std::cout << "=== Alias Analysis Comparison ===\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Total Pointer Pairs: " << dyckResult.totalPairs << "\n\n";
        
        std::cout << "DyckAA:\n";
        std::cout << "  Aliased Pairs: " << dyckResult.aliasedPairs << "\n";
        std::cout << "  Alias Percentage: " << dyckResult.aliasPercentage << "%\n";
        std::cout << "  Analysis Time: " << dyckResult.analysisTime << "s\n\n";
        
        std::cout << "CFL-Anders:\n";
        std::cout << "  Aliased Pairs: " << andersResult.aliasedPairs << "\n";
        std::cout << "  Alias Percentage: " << andersResult.aliasPercentage << "%\n";
        std::cout << "  Analysis Time: " << andersResult.analysisTime << "s\n\n";
        
        std::cout << "CFL-Steens:\n";
        std::cout << "  Aliased Pairs: " << steensResult.aliasedPairs << "\n";
        std::cout << "  Alias Percentage: " << steensResult.aliasPercentage << "%\n";
        std::cout << "  Analysis Time: " << steensResult.analysisTime << "s\n";
    }
};

int main(int argc, char **argv) {
    // Parse command line options
    cl::ParseCommandLineOptions(argc, argv, "Alias Analysis Comparison Tool");
    
    // Process the benchmark path
    std::string expandedBenchmarkPath = expandTilde(BenchmarkPath.getValue());
    std::string absoluteBenchmarkPath = resolveToAbsolutePath(expandedBenchmarkPath);
    
    if (Verbose) {
        std::cout << "Input benchmark path: " << BenchmarkPath.getValue() << std::endl;
        std::cout << "Expanded benchmark path: " << expandedBenchmarkPath << std::endl;
        std::cout << "Absolute benchmark path: " << absoluteBenchmarkPath << std::endl;
    }
    
    // Check if the file exists
    if (!fileExists(absoluteBenchmarkPath)) {
        errs() << "Error: Benchmark file does not exist: " << absoluteBenchmarkPath << "\n";
        return 1;
    }
    
    // Load the LLVM module from bitcode
    if (Verbose) {
        std::cout << "Loading LLVM module from: " << absoluteBenchmarkPath << std::endl;
    }
    
    LLVMContext Context;
    SMDiagnostic Err;
    std::unique_ptr<Module> M = parseIRFile(absoluteBenchmarkPath, Err, Context);
    
    if (!M) {
        errs() << "Error loading module: " << absoluteBenchmarkPath << "\n";
        Err.print(argv[0], errs());
        return 1;
    }

    if (Verbose) {
        std::cout << "Successfully loaded module: " << M->getName().str() << std::endl;
    }
    
    // Create the calculator
    AliasPercentageCalculator calculator;
    
    // Collect all pointer pairs from the module
    std::vector<AliasPercentageCalculator::AliasPair> pairs = calculator.collectPointerPairs(*M);
    
    // Run different alias analyses
    auto dyckResult = calculator.runDyckAA(*M, pairs);
    auto andersResult = calculator.runCFLAndersAA(*M, pairs);
    auto steensResult = calculator.runCFLSteensAA(*M, pairs);
    
    // Print comparison
    calculator.printComparison(dyckResult, andersResult, steensResult);
    
    // Save results to a file
    std::ofstream outFile("alias_comparison_results.txt");
    if (outFile.is_open()) {
        outFile << "=== Alias Analysis Comparison ===\n";
        outFile << std::fixed << std::setprecision(2);
        outFile << "Total Pointer Pairs: " << dyckResult.totalPairs << "\n\n";
        
        outFile << "DyckAA:\n";
        outFile << "  Aliased Pairs: " << dyckResult.aliasedPairs << "\n";
        outFile << "  Alias Percentage: " << dyckResult.aliasPercentage << "%\n";
        outFile << "  Analysis Time: " << dyckResult.analysisTime << "s\n\n";
        
        outFile << "CFL-Anders:\n";
        outFile << "  Aliased Pairs: " << andersResult.aliasedPairs << "\n";
        outFile << "  Alias Percentage: " << andersResult.aliasPercentage << "%\n";
        outFile << "  Analysis Time: " << andersResult.analysisTime << "s\n\n";
        
        outFile << "CFL-Steens:\n";
        outFile << "  Aliased Pairs: " << steensResult.aliasedPairs << "\n";
        outFile << "  Alias Percentage: " << steensResult.aliasPercentage << "%\n";
        outFile << "  Analysis Time: " << steensResult.analysisTime << "s\n";
        
        outFile.close();
        
        std::cout << "\nResults saved to alias_comparison_results.txt\n";
    }
    
    return 0;
} 