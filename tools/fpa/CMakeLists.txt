add_executable(fpa fpa.cpp)

target_link_libraries(fpa
  PRIVATE
  CanaryFPA
  #LLVMBitReader
  LLVMBitWriter
  LLVMIRReader
  #LLVMSupport  
)

install(TARGETS fpa RUNTIME DESTINATION bin)
