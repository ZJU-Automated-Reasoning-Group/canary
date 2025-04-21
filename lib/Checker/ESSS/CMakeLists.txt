set(ESSS_SOURCES
    Analyzer.cc
    CallGraph.cc
    Common.cc
    DataFlowAnalysis.cc
    DebugHelpers.cc
    EHBlockDetector.cc
    ErrorCheckViolationFinder.cc
    FunctionErrorReturnIntervals.cc
    FunctionVSA.cc
    Helpers.cc
    Interval.cc
    MLTA.cc
    PathSpan.cc
)

# Automatically find header files in include directory
file(GLOB ESSS_HEADERS "${CMAKE_SOURCE_DIR}/include/Checker/ESSS/*.h")
add_library(ESSS STATIC ${ESSS_SOURCES})
target_include_directories(ESSS PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Link with LLVM libraries
target_link_libraries(ESSS)

# Install the library
install(TARGETS ESSS
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
) 