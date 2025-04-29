cmake_minimum_required(VERSION 3.10)
project(HiceTools CXX)

# Set C++14 standard
#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Set output directories to keep build artifacts organized
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Ensure object files stay in the build directory
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON)

# Add subdirectories
add_subdirectory(hice-dt)
add_subdirectory(chc_verifier)

# Install targets - updated to use the runtime output directory 
#install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hice-dt
#                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/chc_verifier
#        DESTINATION bin) 