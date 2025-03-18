set(LLVM_LINK_COMPONENTS
        LLVMAggressiveInstCombine
        LLVMAnalysis
        LLVMAsmParser
        LLVMAsmPrinter
        LLVMBinaryFormat
        LLVMBitReader
        LLVMBitWriter
        LLVMBitstreamReader
        LLVMCodeGen
        LLVMCore
        LLVMCoroutines
        LLVMDemangle
        LLVMFrontendOpenMP
        LLVMIRReader
        LLVMInstCombine
        LLVMInstrumentation
        LLVMLTO
        LLVMLinker
        LLVMMC
        LLVMMCParser
        LLVMMIRParser
        LLVMObject
        LLVMObjectYAML
        LLVMOption
        LLVMPasses
        LLVMProfileData
        LLVMRemarks
        LLVMScalarOpts
        LLVMSupport
        LLVMTarget
        LLVMTransformUtils
        LLVMVectorize
        LLVMipo
)

add_executable(canary canary.cpp)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(canary PRIVATE
            CanaryNullPointer CanaryDyckAA CanaryTransform CanarySupport
            # CanarySMT
            -Wl,--start-group
            ${LLVM_LINK_COMPONENTS}
            -Wl,--end-group
            z ncurses pthread dl
    )
else()
    target_link_libraries(canary PRIVATE
            CanaryNullPointer CanaryDyckAA CanaryTransform CanarySupport
            # CanarySMT
            ${LLVM_LINK_COMPONENTS}
            z ncurses pthread dl
    )
endif()

# Add kint executable
add_executable(kint kint.cpp)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(kint PRIVATE
            CanarySupport
            -Wl,--start-group
            ${LLVM_LINK_COMPONENTS}
            -Wl,--end-group
            ${Z3_LIBRARIES} pthread dl
    )
else()
    target_link_libraries(kint PRIVATE
            CanarySupport
            ${LLVM_LINK_COMPONENTS}
            ${Z3_LIBRARIES} pthread dl
    )
endif()

# Find out what libraries are needed by LLVM 
llvm_map_components_to_libnames(LLVM_LINK_COMPONENTS
  AggressiveInstCombine
  Analysis
  AsmParser
  BinaryFormat
  BitReader
  BitWriter
  BitstreamReader
  CodeGen
  Core
  Coroutines
  Demangle
  FrontendOpenMP
  IRReader
  InstCombine
  Instrumentation
  Linker
  MC
  MCParser
  Object
  ProfileData
  Remarks
  ScalarOpts
  Support
  Target
  TransformUtils
  Vectorize
  ipo
)

# Add alias analysis comparison tool
add_executable(alias_analysis alias_analysis.cpp)

# Add the alias_analysis executable
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(alias_analysis PRIVATE
           CanaryDyckAA CFLAA CanarySupport
            -Wl,--start-group
            ${LLVM_LINK_COMPONENTS}
            -Wl,--end-group
            pthread dl
    )
else()
    target_link_libraries(alias_analysis PRIVATE
           CanaryDyckAA CFLAA CanarySupport
            ${LLVM_LINK_COMPONENTS}
            pthread dl
    )
endif()