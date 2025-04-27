set(SOURCES
  Mono/DataFlowAnalysis.cpp
  Mono/DataFlowEngine.cpp
  Mono/DataFlowResult.cpp
  WPDS/DataFlowFacts.cpp
  WPDS/GenKillTransformer.cpp
  WPDS/InterProceduralDataFlowEngine.cpp
)

add_library(DataFlow STATIC ${SOURCES}) 