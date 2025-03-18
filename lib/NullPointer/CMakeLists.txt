add_library(CanaryNullPointer STATIC
        LocalNullCheckAnalysis.cpp
        NullCheckAnalysis.cpp
        NullEquivalenceAnalysis.cpp
        NullFlowAnalysis.cpp
        ContextSensitiveLocalNullCheckAnalysis.cpp
        ContextSensitiveNullCheckAnalysis.cpp
        ContextSensitiveNullFlowAnalysis.cpp
        AliasAnalysisAdapter.cpp
)