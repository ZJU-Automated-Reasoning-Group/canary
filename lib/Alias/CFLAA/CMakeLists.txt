add_library(CFLAA
  CFLAndersAliasAnalysis.cpp
  CFLSteensAliasAnalysis.cpp
)

target_link_libraries(CFLAA
  CanarySupport
  #LLVMCore
  #LLVMSupport
) 

target_include_directories(CFLAA PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/include
)