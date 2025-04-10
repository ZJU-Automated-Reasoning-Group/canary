# Examples CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# Set compiler flags for all targets built in this directory
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wno-deprecated-declarations)
endif()

# Set test directories
set(EXAMPLE_DIRS 
    GenKill
    LiveRegAtoms
    Reach
    csurf
    interrupt
    substrings
    tfa
)

# Add subdirectories for each example
foreach(dir ${EXAMPLE_DIRS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
        add_subdirectory(${dir})
    else()
        message(STATUS "Skipping ${dir} - no CMakeLists.txt file")
    endif()
endforeach()

# Add a check target to run all tests
add_custom_target(check
    COMMENT "Running all examples tests"
)

# Add dependencies on each example test, but only for dirs with CMakeLists.txt
foreach(dir ${EXAMPLE_DIRS})
    if(TARGET check_${dir})
        add_dependencies(check check_${dir})
    endif()
endforeach() 