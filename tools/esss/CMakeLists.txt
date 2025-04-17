set(LLVM_LINK_COMPONENTS
        LLVMPasses
        # Add native target components
        LLVMX86CodeGen
        LLVMX86AsmParser
        # For ARM64 (Apple Silicon)
        LLVMAArch64CodeGen
        LLVMAArch64AsmParser
)

# Add esss executable
add_executable(esss esss_tool.cpp)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(esss PRIVATE
            ESSS CanarySupport
            -Wl,--start-group
            ${LLVM_LINK_COMPONENTS}
            -Wl,--end-group
            z ncurses pthread dl
    )
else()
    target_link_libraries(esss PRIVATE
            ESSS CanarySupport
            ${LLVM_LINK_COMPONENTS}
            z ncurses pthread dl
    )
endif()

# Install the executable
install(TARGETS esss RUNTIME DESTINATION bin) 