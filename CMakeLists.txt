cmake_minimum_required(VERSION 3.10.0)
project(canary C CXX)

# Set C++14 as the standard for all targets in the project
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Check for LLVM build path
if (NOT LLVM_BUILD_PATH)
    message(FATAL_ERROR "Please specify the build folder of LLVM using -DLLVM_BUILD_PATH=")
else()
    message(STATUS "LLVM_BUILD_PATH set to: ${LLVM_BUILD_PATH}")
endif()

# Find LLVM package
find_package(CURL REQUIRED)
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


# Include LLVM directories
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
link_directories(${LLVM_LIBRARY_DIRS})

include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Include RapidJSON
#message(STATUS "Including RapidJSON library")
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/rapidjson)

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

# Handle Boost dependencies for sea-dsa
option(DOWNLOAD_BOOST "Download and build Boost if not found" ON)
set(CUSTOM_BOOST_ROOT "" CACHE PATH "Path to custom boost installation.")

if(CUSTOM_BOOST_ROOT)
    set(BOOST_ROOT ${CUSTOM_BOOST_ROOT})
    set(Boost_NO_SYSTEM_PATHS "ON")
endif()

find_package(Boost 1.65)
if(NOT Boost_FOUND AND DOWNLOAD_BOOST)
    include(ExternalProject)
    set(BOOST_INSTALL_DIR ${CMAKE_BINARY_DIR}/deps/boost)
    message(STATUS "Boost not found, will download and build it in ${BOOST_INSTALL_DIR}")
    
    ExternalProject_Add(
        boost
        GIT_REPOSITORY https://github.com/boostorg/boost.git
        GIT_TAG boost-1.65.0
        PREFIX ${CMAKE_BINARY_DIR}/deps
        CONFIGURE_COMMAND ./bootstrap.sh --prefix=${BOOST_INSTALL_DIR}
        BUILD_COMMAND ./b2 --with-system --with-graph install
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND ""
        INSTALL_DIR ${BOOST_INSTALL_DIR}
    )
    
    set(BOOST_ROOT ${BOOST_INSTALL_DIR})
    set(BOOST_INCLUDEDIR ${BOOST_INSTALL_DIR}/include)
    set(BOOST_LIBRARYDIR ${BOOST_INSTALL_DIR}/lib)
    include_directories(${BOOST_INCLUDEDIR})
else()
    if(Boost_FOUND)
        message(STATUS "Found Boost: ${Boost_INCLUDE_DIRS}")
        include_directories(${Boost_INCLUDE_DIRS})
        if(NOT LLVM_ENABLE_EH)
            add_definitions(-DBOOST_NO_EXCEPTIONS)
        endif()
    else()
        message(FATAL_ERROR "Boost not found and DOWNLOAD_BOOST=OFF. Please provide a valid CUSTOM_BOOST_ROOT or set DOWNLOAD_BOOST=ON.")
    endif()
endif()

# Option for sea-dsa sanity checks
option(ENABLE_SANITY_CHECKS "Enable sea-dsa sanity checks." OFF)
if(ENABLE_SANITY_CHECKS)
  message(STATUS "Compiling sea-dsa with sanity checks")
  set(SANITY_CHECKS TRUE)
else()
  message(STATUS "Compiling sea-dsa without sanity checks")
endif()


if (SVF_DIR)
	set(HAVE_SVF ON)
	add_definitions(-DHAVE_SVF)
	set(SVF_LIBDIR ${SVF_DIR}/lib)

	if (NOT SVF_INCLUDE)
		if (EXISTS "${SVF_DIR}/include/WPA/Andersen.h")
			set(SVF_INCLUDE ${SVF_DIR}/include)
		elseif (EXISTS "${SVF_DIR}/../include/WPA/Andersen.h")
			set(SVF_INCLUDE ${SVF_DIR}/../include)
		else()
			message(FATAL_ERROR "Did not find the directory with SVF headers")
		endif()
	endif()

	set(SVF_LIBS Svf Cudd)

	include_directories(SYSTEM ${SVF_INCLUDE})
	link_directories(${SVF_LIBDIR} ${SVF_LIBDIR}/CUDD)

	if (NOT LLVM_LINK_DYLIB)
		if (${LLVM_PACKAGE_VERSION} VERSION_GREATER "3.4")
			llvm_map_components_to_libnames(llvm_transformutils transformutils)
		else()
			llvm_map_components_to_libraries(llvm_transformutils transformutils)
		endif()
	endif()

	message(STATUS "SVF dir: ${SVF_DIR}")
	message(STATUS "SVF libraries dir: ${SVF_LIBDIR}")
	message(STATUS "SVF include dir: ${SVF_INCLUDE}")
	message(STATUS "SVF libs: ${SVF_LIBS}")
endif(SVF_DIR)


# Add subdirectories
add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(benchmarks)

option(ENABLE_EXAMPLES "Enable examples" OFF)
if(ENABLE_EXAMPLES)
	add_subdirectory(examples)
endif()
