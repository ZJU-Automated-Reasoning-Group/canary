# Examples CMakeLists.txt

# BDD Example
add_executable(BDDExample BDDExample.cpp)
target_include_directories(BDDExample PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(BDDExample PRIVATE CanaryCUDD BDD m)

# Model Checking Example
add_executable(ModelCheckingExample ModelCheckingExample.cpp)
target_include_directories(ModelCheckingExample PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ModelCheckingExample PRIVATE CanaryCUDD BDD m) 