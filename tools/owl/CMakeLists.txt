set(LLVM_LINK_COMPONENTS
        #LLVMDemangle
        LLVMSupport
)

add_executable(owl owl.cpp)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(owl PRIVATE
            CanarySMT
            ${LLVM_LINK_COMPONENTS}
            ${Z3_LIBRARIES}
            -Wl,--start-group
            -Wl,--end-group
            z ncurses pthread dl
    )
else()
    target_link_libraries(owl PRIVATE
            CanarySMT
            ${LLVM_LINK_COMPONENTS}
            ${Z3_LIBRARIES}
            z ncurses pthread dl
    )
endif()