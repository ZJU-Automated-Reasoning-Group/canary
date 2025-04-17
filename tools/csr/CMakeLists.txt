add_executable(csr csr.cpp)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(csr PRIVATE
            CanaryCSIndex
            -Wl,--start-group
            -Wl,--end-group
            z ncurses pthread dl
    )
else()
    target_link_libraries(csr PRIVATE
            CanaryCSIndex
            z ncurses pthread dl
    )
endif()