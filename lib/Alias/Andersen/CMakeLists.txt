include_directories(
	${andersen_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/include/Alias/Andersen
)

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

target_link_libraries(Andersen
	LLVMCore
	LLVMSupport
	LLVMAnalysis
	LLVMTransformUtils
	# BDD
)