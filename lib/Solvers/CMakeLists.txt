add_subdirectory(SMT)
add_subdirectory(CUDD)

# Add BDD wrapper
add_library(BDD STATIC BDD.cpp)
target_include_directories(BDD PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(BDD PRIVATE CanaryCUDD) 