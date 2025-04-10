# Source files for the merged WPDS library
set(WPDS_SRCS
    dictionary.cpp
    common.cpp
    keys.cpp
    key_source.cpp
)

# Create libraries
add_library(wpds STATIC ${WPDS_SRCS})
add_library(wpds++ STATIC ${WPDS_SRCS})

# Set compiler flags to suppress warnings related to deprecated functions
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(wpds PRIVATE -Wno-deprecated-declarations)
    target_compile_options(wpds++ PRIVATE -Wno-deprecated-declarations)
endif()

# Include directories
target_include_directories(wpds PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/Solvers/WPDS
)
target_include_directories(wpds++ PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/Solvers/WPDS
)

# Set compile definitions for wpds++
target_compile_definitions(wpds++ PRIVATE WITNESS_TRACE=1 GRAMMATECH=1)

# Create interface libraries for header-only components
add_library(ewpds INTERFACE)
target_include_directories(ewpds INTERFACE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/Solvers/WPDS
)

add_library(wpdsplusplus_util INTERFACE)
target_include_directories(wpdsplusplus_util INTERFACE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/Solvers/WPDS
)

# Install headers
file(GLOB_RECURSE WPDS_HEADERS "${CMAKE_SOURCE_DIR}/include/Solvers/WPDS/*.h")
install(FILES ${WPDS_HEADERS} DESTINATION include/Solvers/WPDS)

# Install targets
install(TARGETS wpds wpds++ ewpds wpdsplusplus_util
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib) 