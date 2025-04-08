#pragma once

#include <llvm/Support/CommandLine.h>

enum MLTAMode {
    NoIndirectCalls,
    MatchSignatures,
    FullMLTA
};

#if 1
extern cl::opt<float> AssociationConfidence;
extern cl::opt<float> IntervalConfidenceThreshold;
extern cl::opt<bool> ShowSafetyChecks;
extern cl::opt<unsigned int> PrintRandomNonVoidFunctionSamples;
extern cl::opt<bool> RefineWithVSA;
extern cl::opt<float> IncorrectCheckThreshold;
extern cl::opt<float> MissingCheckThreshold;
#endif
extern cl::opt<MLTAMode> MLTA;
extern cl::opt<string> FunctionTestCasesToAnalyze;
extern cl::opt<unsigned int> ThreadCount;

struct GlobalContext;
extern GlobalContext GlobalCtx;