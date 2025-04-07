add_library(CanaryDyckAA STATIC
        AAAnalyzer.cpp
        DyckAliasAnalysis.cpp
        DyckCallGraph.cpp
        DyckCallGraphNode.cpp
        DyckGraph.cpp
        DyckGraphNode.cpp
        DyckModRefAnalysis.cpp
        DyckValueFlowAnalysis.cpp
        DyckVFG.cpp
        MRAnalyzer.cpp
)

# Set include directories
target_include_directories(CanaryDyckAA PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/include
)
