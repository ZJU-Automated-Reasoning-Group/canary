//===- dataflow-tool.cpp - Driver for dataflow analyses -------------------===//
//
// Part of the Canary Project
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements a command-line tool for running dataflow analyses.
///
//===----------------------------------------------------------------------===//

#include "Dataflow/LivenessAnalysis.h"
#include "Dataflow/ReachingDefinitions.h"
#include "Support/RecursiveTimer.h"
#include "Support/Statistics.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils.h"

#include <memory>
#include <string>
#include <vector>
#include <chrono>

using namespace llvm;
using namespace canary;

enum AnalysisType {
  LIVENESS,
  REACHING_DEFS,
  ALL
};

// Command line options
static cl::opt<std::string> InputFilename(cl::Positional, cl::desc("<input bitcode file>"),
                                          cl::init("-"), cl::value_desc("filename"));

static cl::opt<std::string> OutputFilename("o", cl::desc("Output file for analysis results"),
                                           cl::value_desc("filename"));

static cl::opt<AnalysisType> Analysis("analysis", cl::desc("Choose the analysis to run"),
                                    cl::values(
                                      clEnumValN(LIVENESS, "liveness", "Run liveness analysis"),
                                      clEnumValN(REACHING_DEFS, "reaching-defs", "Run reaching definitions analysis"),
                                      clEnumValN(ALL, "all", "Run all analyses")
                                    ),
                                    cl::init(ALL));

static cl::list<std::string> Functions("function", cl::desc("Specify function(s) to analyze"),
                                     cl::value_desc("function name"), cl::CommaSeparated);

static cl::opt<bool> TimeAnalysis("time", cl::desc("Time the analysis execution"),
                                cl::init(false));

static cl::opt<bool> Verbose("v", cl::desc("Show detailed analysis results"),
                           cl::init(false));

static cl::opt<unsigned> MaxFunctionSize("max-instrs", cl::desc("Only analyze functions with fewer than this many instructions"),
                                       cl::init(1000));

// Helper function to run liveness analysis
void runLivenessAnalysis(Function &F, raw_ostream &OS) {
  OS << "Running liveness analysis on function '" << F.getName() << "'...\n";
  
  auto startTime = std::chrono::high_resolution_clock::now();
  LivenessAnalysis LVA(F);
  LVA.run();
  auto endTime = std::chrono::high_resolution_clock::now();
  
  if (TimeAnalysis) {
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    OS << "Liveness analysis completed in " << duration.count() / 1000.0 << " ms\n";
  }
  
  if (Verbose) {
    LVA.print(OS);
  } else {
    OS << "Liveness analysis completed on '" << F.getName() << "'\n";
    
    // Print summary statistics
    unsigned totalBlocks = 0;
    unsigned totalLiveVars = 0;
    
    for (auto &BB : F) {
      totalBlocks++;
      const LivenessDomain &Domain = LVA.getResult(&BB);
      totalLiveVars += Domain.getLiveVars().size();
    }
    
    OS << "  Summary: " << totalBlocks << " basic blocks, average " 
       << (totalBlocks > 0 ? (double)totalLiveVars / totalBlocks : 0) 
       << " live variables per block\n";
  }
}

// Helper function to run reaching definitions analysis
void runReachingDefsAnalysis(Function &F, raw_ostream &OS) {
  OS << "Running reaching definitions analysis on function '" << F.getName() << "'...\n";
  
  auto startTime = std::chrono::high_resolution_clock::now();
  ReachingDefsAnalysis RDA(F);
  RDA.run();
  auto endTime = std::chrono::high_resolution_clock::now();
  
  if (TimeAnalysis) {
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    OS << "Reaching definitions analysis completed in " << duration.count() / 1000.0 << " ms\n";
  }
  
  if (Verbose) {
    RDA.print(OS);
  } else {
    OS << "Reaching definitions analysis completed on '" << F.getName() << "'\n";
    
    // Print summary statistics
    unsigned totalBlocks = 0;
    unsigned totalDefinitions = 0;
    
    for (auto &BB : F) {
      totalBlocks++;
      const ReachingDefsDomain &Domain = RDA.getResult(&BB);
      
      // Count total definitions
      for (const auto &Pair : Domain.getAllDefinitions()) {
        totalDefinitions += Pair.second.size();
      }
    }
    
    OS << "  Summary: " << totalBlocks << " basic blocks, " 
       << totalDefinitions << " total reaching definitions, average " 
       << (totalBlocks > 0 ? (double)totalDefinitions / totalBlocks : 0) 
       << " definitions per block\n";
  }
}

int main(int argc, char **argv) {
  // Initialize LLVM
  InitLLVM X(argc, argv);
  
  // Enable debug stream buffering
  EnableDebugBuffering = true;
  
  // Print a stack trace if a fatal signal occurs
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  
  // Parse command line options
  cl::ParseCommandLineOptions(argc, argv, "Canary Dataflow Analysis Tool\n\n"
                              "This tool runs different dataflow analyses on LLVM bitcode files.\n");
  
  // Set up the context
  LLVMContext Context;
  SMDiagnostic Err;
  
  // Parse the input file
  std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
  if (!M) {
    Err.print(argv[0], errs());
    return 1;
  }
  
  // Verify the module
  if (verifyModule(*M, &errs())) {
    errs() << argv[0] << ": error: input module is broken!\n";
    return 1;
  }
  
  // Prepare the output stream
  std::unique_ptr<ToolOutputFile> Out;
  if (!OutputFilename.empty()) {
    std::error_code EC;
    Out = std::make_unique<ToolOutputFile>(OutputFilename, EC, sys::fs::OF_None);
    if (EC) {
      errs() << "Error opening output file: " << EC.message() << '\n';
      return 1;
    }
  }
  
  raw_ostream &OS = Out ? Out->os() : outs();
  
  // Store functions to analyze
  std::vector<Function*> FunctionsToAnalyze;
  
  // If specific functions are requested, use only those
  if (!Functions.empty()) {
    for (const auto &FnName : Functions) {
      if (Function *F = M->getFunction(FnName)) {
        FunctionsToAnalyze.push_back(F);
      } else {
        errs() << "Warning: Function '" << FnName << "' not found in module\n";
      }
    }
  } else {
    // Otherwise analyze all non-empty functions smaller than the max size
    for (Function &F : *M) {
      if (!F.isDeclaration() && F.size() < MaxFunctionSize) {
        FunctionsToAnalyze.push_back(&F);
      }
    }
  }
  
  // Check if we have functions to analyze
  if (FunctionsToAnalyze.empty()) {
    errs() << "Error: No functions to analyze\n";
    return 1;
  }
  
  OS << "Analyzing " << FunctionsToAnalyze.size() << " function(s) from " 
     << InputFilename << "\n";
  
  // Run the requested analyses
  for (Function *F : FunctionsToAnalyze) {
    OS << "\n";
    
    if (Analysis == LIVENESS || Analysis == ALL) {
      runLivenessAnalysis(*F, OS);
    }
    
    if (Analysis == REACHING_DEFS || Analysis == ALL) {
      runReachingDefsAnalysis(*F, OS);
    }
  }
  
  // Keep the output file
  if (Out) {
    Out->keep();
  }
  
  return 0;
} 