set(LLVM_LINK_COMPONENTS
        LLVMPasses
)

add_executable(kint kint.cpp)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(kint PRIVATE
            CanarySupport
            -Wl,--start-group
            ${LLVM_LINK_COMPONENTS}
            -Wl,--end-group
            ${Z3_LIBRARIES} pthread dl
    )
else()
    target_link_libraries(kint PRIVATE
            CanarySupport
            ${LLVM_LINK_COMPONENTS}
            ${Z3_LIBRARIES} pthread dl
    )
endif()

