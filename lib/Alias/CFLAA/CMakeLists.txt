add_library(CFLAA
  CFLAndersAliasAnalysis.cpp
  CFLSteensAliasAnalysis.cpp
)

target_link_libraries(CFLAA
  CanarySupport
) 