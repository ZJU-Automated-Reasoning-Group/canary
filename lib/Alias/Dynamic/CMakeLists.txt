add_library(CanaryDynamicAA STATIC
        DynamicAliasAnalysis.cpp
        DynamicHooks.cpp
        FeatureCheck.cpp
        IDAssigner.cpp
        LogPrinter.cpp
        LogReader.cpp
        MemoryInstrument.cpp
)

# Create the runtime library that will be linked with instrumented programs
add_library(Runtime STATIC
        MemoryHooks.c
)

# Export libRuntime.a to the bin directory for easy access
set_target_properties(Runtime PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        OUTPUT_NAME "Runtime"
)