# GenKill example CMakeLists.txt

# Common source file
set(COMMON_SRC VarSet.cpp)

# Set platform specific extension
if(WIN32)
    set(EXE_SUFFIX ".exe")
else()
    set(EXE_SUFFIX "")
endif()

# Define executables
set(EXECUTABLES
    uninit_var
    live_var
    test
    uninit_var.pre.o.post
)

# Define debug versions with -DDWPDS flag
set(DEBUG_EXECUTABLES
    duninit_var
    dlive_var
    duninit_var.pre.o.post
)

# Build regular executables
foreach(exec ${EXECUTABLES})
    add_executable(${exec} ${exec}.cpp ${COMMON_SRC})
    target_link_libraries(${exec} wpds)
    target_include_directories(${exec} PRIVATE ${CMAKE_SOURCE_DIR})
    target_compile_options(${exec} PRIVATE -DPRIORITY_PATH_SUMMARY)
endforeach()

# Build debug executables
foreach(exec ${DEBUG_EXECUTABLES})
    string(SUBSTRING ${exec} 1 -1 base_exec)
    add_executable(${exec} ${base_exec}.cpp ${COMMON_SRC})
    target_link_libraries(${exec} wpds)
    target_include_directories(${exec} PRIVATE ${CMAKE_SOURCE_DIR})
    target_compile_options(${exec} PRIVATE -DDWPDS -DPRIORITY_PATH_SUMMARY)
endforeach()

# Add check target for this directory
add_custom_target(check_GenKill
    COMMENT "Checking GenKill examples"
    DEPENDS ${EXECUTABLES} ${DEBUG_EXECUTABLES}
)

# Add test targets for each executable
foreach(exec ${EXECUTABLES} ${DEBUG_EXECUTABLES})
    add_custom_command(TARGET check_GenKill POST_BUILD
        COMMAND ${exec} > ${exec}.out.txt
        COMMAND ${CMAKE_COMMAND} -E compare_files ${exec}.out.txt ${CMAKE_CURRENT_SOURCE_DIR}/baselines/${exec}.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running and checking ${exec}"
    )
endforeach()

# Add baseline generation target
add_custom_target(baseline_GenKill
    COMMENT "Generating baselines for GenKill examples"
    DEPENDS ${EXECUTABLES} ${DEBUG_EXECUTABLES}
)

foreach(exec ${EXECUTABLES} ${DEBUG_EXECUTABLES})
    add_custom_command(TARGET baseline_GenKill POST_BUILD
        COMMAND ${exec} > ${CMAKE_CURRENT_SOURCE_DIR}/baselines/${exec}.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating baseline for ${exec}"
    )
endforeach() 