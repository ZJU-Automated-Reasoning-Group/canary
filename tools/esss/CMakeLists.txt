set(LLVM_LINK_COMPONENTS
  LLVMBitWriter
  LLVMIRReader
)

add_executable(esss esss.cpp)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(esss
      PRIVATE
      CanaryESSS
      CanarySupport
      -Wl,--start-group
      ${LLVM_LINK_COMPONENTS}
      -Wl,--end-group
      pthread dl
    )
else()
    target_link_libraries(esss
      PRIVATE
      CanaryESSS
      CanarySupport
      ${LLVM_LINK_COMPONENTS}
      pthread dl
    )
endif()