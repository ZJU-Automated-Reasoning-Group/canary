# Reach example CMakeLists.txt

set(SOURCES
    ExReach.cpp
    Reach.cpp
)

add_executable(ExReach ${SOURCES})
target_link_libraries(ExReach wpds)
target_include_directories(ExReach PRIVATE ${CMAKE_SOURCE_DIR})

# Add test targets
add_custom_target(run_ExReach
    COMMAND ExReach > ExReach.out.txt
    DEPENDS ExReach
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running ExReach example"
)

add_custom_target(check_Reach
    COMMAND ${CMAKE_COMMAND} -E compare_files 
            ${CMAKE_CURRENT_BINARY_DIR}/ExReach.out.txt 
            ${CMAKE_CURRENT_SOURCE_DIR}/baselines/ExReach.txt
    DEPENDS run_ExReach
    COMMENT "Checking ExReach example output against baseline"
)

# Add custom target to generate baseline
add_custom_target(baseline_Reach
    COMMAND ExReach > ${CMAKE_CURRENT_SOURCE_DIR}/baselines/ExReach.txt
    DEPENDS ExReach
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating baseline for ExReach example"
) 