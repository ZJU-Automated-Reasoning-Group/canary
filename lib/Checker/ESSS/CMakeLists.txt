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

set(ESSS_HEADERS
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/Analyzer.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/CallGraph.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/CIOptForward.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/ClOptForward.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/Common.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/DataFlowAnalysis.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/DebugHelpers.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/EHBlockDetector.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/ErrorCheckViolationFinder.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/FunctionErrorReturnIntervals.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/FunctionVSA.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/Helpers.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/Interval.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/Lazy.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/MLTA.h
    ${CMAKE_SOURCE_DIR}/include/Checker/ESSS/PathSpan.h
)

add_library(ESSS STATIC ${ESSS_SOURCES} ${ESSS_HEADERS})
target_include_directories(ESSS PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Link with LLVM libraries
target_link_libraries(ESSS
    LLVMAsmParser
    LLVMSupport
    LLVMCore
    LLVMAnalysis
    LLVMIRReader
    LLVMDemangle
)

# Install the library
install(TARGETS ESSS
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
) 