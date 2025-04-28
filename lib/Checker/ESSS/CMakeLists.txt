add_library(CanaryESSS STATIC
        CallGraph.cpp
        Common.cpp
        DataFlowAnalysis.cpp
        DebugHelpers.cpp
        EHBlockDetector.cpp
        ErrorCheckViolationFinder.cpp
        FunctionErrorReturnIntervals.cpp
        FunctionVSA.cpp
        Helpers.cpp
        Interval.cpp
        MLTA.cpp
)