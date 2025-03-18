cmake_minimum_required(VERSION 3.10.0)
project(canary C CXX)

# Set C++ standard to 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Check for LLVM build path
if (NOT LLVM_BUILD_PATH)
    message(FATAL_ERROR "Please specify the build folder of LLVM using -DLLVM_BUILD_PATH=")
else()
    message(STATUS "LLVM_BUILD_PATH set to: ${LLVM_BUILD_PATH}")
endif()

# Find LLVM package
find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_BUILD_PATH})

if(NOT LLVM_FOUND)
    message(FATAL_ERROR "Cannot find LLVMConfig.cmake under path: ${LLVM_BUILD_PATH}")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM header files located in: ${LLVM_INCLUDE_DIRS}")

# Check LLVM version
if (LLVM_PACKAGE_VERSION VERSION_EQUAL "12.0.0")
    message(STATUS "Using LLVM version 12")
    add_definitions(-DLLVM12)
elseif (LLVM_PACKAGE_VERSION VERSION_EQUAL "14.0.0")
    message(STATUS "Using LLVM version 14")
    add_definitions(-DLLVM14)
else()
    message(FATAL_ERROR "Unsupported or unknown LLVM version: ${LLVM_PACKAGE_VERSION}")
endif()

# Set compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fexceptions")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(BUILD_TESTS "build all unit tests" OFF)

# Include LLVM directories
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
link_directories(${LLVM_LIBRARY_DIRS})

include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find Z3
find_library(
    Z3_LIBRARIES
    NAMES z3
    HINTS ${Z3_DIR} ENV Z3_DIR
    PATH_SUFFIXES bin lib
)
find_path(
    Z3_INCLUDES
    NAMES z3++.h
    HINTS ${Z3_DIR} ENV Z3_DIR
    PATH_SUFFIXES include z3
)
if(NOT Z3_LIBRARIES OR NOT Z3_INCLUDES)
    message(FATAL_ERROR "Z3 not found!")
endif()
message(STATUS "Found Z3: ${Z3_LIBRARIES}")
message(STATUS "Z3 include dir: ${Z3_INCLUDES}")

include_directories(${Z3_INCLUDES})

# Add subdirectories
add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(benchmarks)

if (BUILD_TESTS)
    add_subdirectory(test)
endif()

enable_testing()
add_test(AliasTest ${PROJECT_BINARY_DIR}/test/AliasTest)