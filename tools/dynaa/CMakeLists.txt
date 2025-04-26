add_executable(dynaa-check dynaa-check.cpp)
target_link_libraries(dynaa-check CanaryDynamicAA LLVMAnalysis LLVMIRReader)

add_executable(dynaa dynaa.cpp)
target_link_libraries(dynaa CanaryDynamicAA LLVMSupport)

add_executable(dynaa-log-dump dynaa-log-dump.cpp)
target_link_libraries(dynaa-log-dump CanaryDynamicAA)

add_executable(dynaa-instrument dynaa-instrument.cpp)
target_link_libraries(dynaa-instrument CanaryDynamicAA LLVMIRReader LLVMBitWriter)