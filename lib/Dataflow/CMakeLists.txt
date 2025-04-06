add_library(DataflowAnalysis
  DataflowAnalysis.cpp
  LivenessAnalysis.cpp
  ReachingDefinitions.cpp
)

target_link_libraries(DataflowAnalysis
  LLVMCore
  LLVMSupport
) 