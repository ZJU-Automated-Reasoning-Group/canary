set(LLVM_LINK_COMPONENTS
        LLVMPasses       
)

add_executable(kint kint.cpp)

# Set C++14 standard specifically for kint
target_compile_features(kint PRIVATE cxx_std_14)

# Ensure we're using C++14 mode on all compilers 
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(kint PRIVATE -std=c++14)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(kint PRIVATE /std:c++14)
endif()

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