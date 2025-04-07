# Configure seadsa config header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../include/seadsa/config.h.cmake
  ${CMAKE_BINARY_DIR}/include/seadsa/config.h)

# Create seadsa library
add_library(SeaDsaAnalysis
  Graph.cc
  FieldType.cc
  DsaLocal.cc
  DsaGlobal.cc
  DsaCallSite.cc
  Cloner.cc
  DsaInfo.cc
  DsaStats.cc
  Mapper.cc
  DsaBottomUp.cc
  DsaTopDown.cc
  DsaAnalysis.cc
  DsaPrinter.cc
  CallGraphWrapper.cc
  TypeUtils.cc
  Debug.cc
  NameValues.cc
  RemovePtrToInt.cc
  AllocWrapInfo.cc
  AllocSiteInfo.cc
  DsaAllocSite.cc
  DsaCompleteCallGraph.cc
  DsaCallGraphPrinter.cc
  DsaColor.cc
  ShadowMem.cc
  SeaMemorySSA.cc
  SeaDsaAliasAnalysis.cc
  DsaLibFuncInfo.cc
  InitializePasses.cc
)

# Link against LLVM libraries
target_link_libraries(SeaDsaAnalysis PUBLIC
  ${LLVM_LIBRARIES}
)

# Set include directories
target_include_directories(SeaDsaAnalysis PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
  ${CMAKE_BINARY_DIR}/include
)

# Set compile definitions
if (SANITY_CHECKS)
  target_compile_definitions(SeaDsaAnalysis PRIVATE SEADSA_SANITY_CHECKS)
endif()

# Install library
install(TARGETS SeaDsaAnalysis
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
