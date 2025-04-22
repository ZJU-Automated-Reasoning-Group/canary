set(AndersenSourceCodes
	Andersen.cpp
	AndersenAA.cpp
	ConstraintCollect.cpp
	ConstraintOptimize.cpp
	ConstraintSolving.cpp
	ExternalLibrary.cpp
	NodeFactory.cpp
)

add_library(AndersenObj OBJECT ${AndersenSourceCodes})
add_library(Andersen SHARED $<TARGET_OBJECTS:AndersenObj>)
add_library(AndersenStatic STATIC $<TARGET_OBJECTS:AndersenObj>)
set_target_properties(AndersenStatic PROPERTIES OUTPUT_NAME "Andersen")

target_link_libraries(Andersen
	LLVMCore
)

target_link_libraries(AndersenStatic
	LLVMCore
)