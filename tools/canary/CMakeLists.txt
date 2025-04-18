# Find out what libraries are needed by LLVM
llvm_map_components_to_libnames(LLVM_LINK_COMPONENTS
  Coroutines
  Support
  Target
  TransformUtils
)

add_executable(canary canary.cpp)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(canary PRIVATE
            CanaryNullPointer CanaryDyckAA CanaryTransform CanarySupport
            # CanarySMT
            -Wl,--start-group
            ${LLVM_LINK_COMPONENTS}
            -Wl,--end-group
            z ncurses pthread dl
    )
else()
    target_link_libraries(canary PRIVATE
            CanaryNullPointer CanaryDyckAA CanaryTransform CanarySupport
            # CanarySMT
            ${LLVM_LINK_COMPONENTS}
            z ncurses pthread dl
    )
endif()