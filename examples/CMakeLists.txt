# Examples CMakeLists.txt

# BDD Example
add_executable(BDDExample BDDExample.cpp)
target_include_directories(BDDExample PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(BDDExample PRIVATE CanaryCUDD BDD m)


# Model Checking Example
add_executable(ModelCheckingExample ModelCheckingExample.cpp)
target_include_directories(ModelCheckingExample PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ModelCheckingExample PRIVATE CanaryCUDD BDD m)

# RapidJSON Example
add_executable(RapidJSONExample RapidJSONExample.cpp)
target_include_directories(RapidJSONExample PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Andersen Example
add_executable(AndersenExample AndersenExample.cpp)
target_include_directories(AndersenExample PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Get LLVM components needed for this example
llvm_map_components_to_libnames(llvm_libs support core irreader analysis)

target_link_libraries(AndersenExample PRIVATE 
  AndersenStatic 
  ${llvm_libs}
) 

# BDD Points-to Set Benchmark
add_executable(BDDPtsSetBenchmark BDDPtsSetBenchmark.cpp)
target_include_directories(BDDPtsSetBenchmark PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(BDDPtsSetBenchmark PRIVATE
  AndersenStatic
  CanaryCUDD
  BDD
  ${llvm_libs}
  m
) 

# WPDS Examples 
add_subdirectory(wpds) 