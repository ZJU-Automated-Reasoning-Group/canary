# Source files for the WPDS libraries
set(WPDS_SRCS
    dictionary.cpp
    common.cpp
    keys.cpp
    key_source.cpp
)

# Define include directories
set(WPDS_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/Solvers/WPDS
)

# Create libraries
add_library(wpds STATIC ${WPDS_SRCS})
add_library(wpds++ STATIC ${WPDS_SRCS})

# Configure common properties
foreach(target wpds wpds++)
    target_include_directories(${target} PUBLIC ${WPDS_INCLUDE_DIRS})
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wno-deprecated-declarations)
    endif()
endforeach()

# Set specific compile definitions for wpds++
target_compile_definitions(wpds++ PRIVATE WITNESS_TRACE=1 GRAMMATECH=1)

# Create interface libraries for header-only components
foreach(target ewpds wpdsplusplus_util)
    add_library(${target} INTERFACE)
    target_include_directories(${target} INTERFACE ${WPDS_INCLUDE_DIRS})
endforeach()

# Install headers
file(GLOB_RECURSE WPDS_HEADERS "${CMAKE_SOURCE_DIR}/include/Solvers/WPDS/*.h")
install(FILES ${WPDS_HEADERS} DESTINATION include/Solvers/WPDS)

# Install targets
install(TARGETS wpds wpds++ ewpds wpdsplusplus_util
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib) 