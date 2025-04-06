add_executable(dataflow-tool
  dataflow-tool.cpp
)

llvm_map_components_to_libnames(LLVM_LIBS
  irreader
  bitwriter
  core
  support
  ipo
  transformutils
  instcombine
  passes
)

target_link_libraries(dataflow-tool
  PRIVATE
  ${LLVM_LIBS}
  DataflowAnalysis
)

set_target_properties(dataflow-tool PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS dataflow-tool
  RUNTIME DESTINATION bin
) 