add_executable(seadsa-dg sea-dsa-dg.cpp)
target_link_libraries(seadsa-dg PRIVATE 
  SeaDsaAnalysis
)

add_executable(seadsa-tool seadsa-tool.cpp)
target_link_libraries(seadsa-tool PRIVATE
  SeaDsaAnalysis
) 