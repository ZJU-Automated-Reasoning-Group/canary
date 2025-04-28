add_executable(fpa fpa.cpp)

target_link_libraries(fpa
  PRIVATE
  CanaryFPA
  LLVMBitWriter
  LLVMIRReader
)

install(TARGETS fpa RUNTIME DESTINATION bin)
