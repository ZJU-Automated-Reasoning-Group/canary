set (AnnotationSourceCodes
	ExternalModRefTable.cpp
	ExternalPointerTable.cpp
)
add_library (Annotation STATIC ${AnnotationSourceCodes})
