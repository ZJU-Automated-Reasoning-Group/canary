cmake_minimum_required(VERSION 3.10)
project(z_solver)

# Add these lines to set C++ standard before finding packages
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

# allow for set CMAKE_BUILD_TYPE to be Debug or Release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()
# set the default build type to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Z3 REQUIRED)
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${Z3_INCLUDE_DIRS}
)

# Create a library for the frontend components
add_library(frontend_lib
    "frontend/cParser.h"
    "frontend/cParser.cpp"
    "frontend/SyGuSParser.h"
    "frontend/SyGuSParser.cpp"
)

# Add the C to CHC frontend binary
add_executable(c2chc
    "frontend/c2chc.cpp"
)

# Add the SyGuS to CHC frontend binary
add_executable(sygus2chc
    "frontend/sygus2chc.cpp"
)

# Link the frontend binaries with the frontend library
target_link_libraries(c2chc
    frontend_lib
    ${Z3_LIBRARIES}
)

target_link_libraries(sygus2chc
    frontend_lib
    ${Z3_LIBRARIES}
)

add_executable(bestInv
 "bestInvMain.cpp" "parameterParser.h" "parameterParser.cpp" "utils/zUtils.h" "utils/zUtils.cpp" "csvDealer.h" "csvDealer.cpp" "bestInv/invTemplate.h" "bestInv/UBVIntervalInvariant.cpp" "bestInv/transitionSystem.h" "bestInv/transitionSystem.cpp" "bestInv/chcParser.cpp" "bestInv/chcParser.h" "bestInv/invTemplate.cpp" "bestInv/UBVOctagonInvariant.cpp" "bestInv/ABVBitwiseInvariant.cpp" "modelSolver/assertionDatabase.h"  "modelSolver/assertionDatabase.cpp" "modelSolver/generalExpr.h" "modelSolver/generalExpr.cpp" "parallelSolver/baseSolver.h" "parallelSolver/baseSolver.cpp" "parallelSolver/clusterSolver.h" "parallelSolver/clusterSolver.cpp"
)

target_link_libraries(bestInv
    ${Z3_LIBRARIES}
    frontend_lib
)

target_include_directories(bestInv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(kInductor
 "kInductorMain.cpp" "parameterParser.h" "parameterParser.cpp" "utils/zUtils.h" "utils/zUtils.cpp" "csvDealer.h" "csvDealer.cpp" "bestInv/invTemplate.h" "bestInv/UBVIntervalInvariant.cpp" "bestInv/transitionSystem.h" "bestInv/transitionSystem.cpp" "bestInv/chcParser.h" "bestInv/chcParser.cpp" "bestInv/invTemplate.cpp" "bestInv/UBVOctagonInvariant.cpp" "bestInv/ABVBitwiseInvariant.cpp" "modelSolver/assertionDatabase.h"  "modelSolver/assertionDatabase.cpp" "modelSolver/generalExpr.h" "modelSolver/generalExpr.cpp" "parallelSolver/baseSolver.h" "parallelSolver/baseSolver.cpp" "parallelSolver/clusterSolver.h" "parallelSolver/clusterSolver.cpp" "kInduction/kInductor.h" "kInduction/kInductor.cpp"
)

target_link_libraries(kInductor
    ${Z3_LIBRARIES}
    frontend_lib
)

target_include_directories(kInductor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

