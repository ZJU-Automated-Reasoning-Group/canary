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
        LLVMDebugInfoCodeView
        LLVMDebugInfoDWARF
        LLVMDebugInfoGSYM
        LLVMDebugInfoMSF
        LLVMDebugInfoPDB
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
        # Add native target components
        LLVMX86CodeGen
        LLVMX86AsmParser
        LLVMX86Desc
        LLVMX86Disassembler
        LLVMX86Info
        # For ARM64 (Apple Silicon)
        LLVMAArch64CodeGen
        LLVMAArch64AsmParser
        LLVMAArch64Desc
        LLVMAArch64Disassembler
        LLVMAArch64Info
        # Add target libraries
        LLVMAArch64Info
        LLVMAArch64Desc
        LLVMAArch64AsmParser
        LLVMAMDGPUInfo
        LLVMAMDGPUDesc
        LLVMAMDGPUAsmParser
        LLVMARMInfo
        LLVMARMDesc
        LLVMARMAsmParser
        LLVMAVRInfo
        LLVMAVRDesc
        LLVMAVRAsmParser
        LLVMBPFInfo
        LLVMBPFDesc
        LLVMBPFAsmParser
        LLVMHexagonInfo
        LLVMHexagonDesc
        LLVMHexagonAsmParser
        LLVMLanaiInfo
        LLVMLanaiDesc
        LLVMLanaiAsmParser
        LLVMMSP430Info
        LLVMMSP430Desc
        LLVMMSP430AsmParser
        LLVMMipsInfo
        LLVMMipsDesc
        LLVMMipsAsmParser
        LLVMNVPTXInfo
        LLVMNVPTXDesc
        LLVMPowerPCInfo
        LLVMPowerPCDesc
        LLVMPowerPCAsmParser
        LLVMRISCVInfo
        LLVMRISCVDesc
        LLVMRISCVAsmParser
        LLVMSparcInfo
        LLVMSparcDesc
        LLVMSparcAsmParser
        LLVMSystemZInfo
        LLVMSystemZDesc
        LLVMSystemZAsmParser
        LLVMVEInfo
        LLVMVEDesc
        LLVMVEAsmParser
        LLVMWebAssemblyInfo
        LLVMWebAssemblyDesc
        LLVMWebAssemblyAsmParser
        LLVMXCoreInfo
        LLVMXCoreDesc
        LLVMDebuginfod
)

# Add esss executable
add_executable(esss esss_tool.cpp)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(esss PRIVATE
            ESSS CanarySupport
            -Wl,--start-group
            ${LLVM_LINK_COMPONENTS}
            -Wl,--end-group
            z ncurses pthread dl
    )
else()
    target_link_libraries(esss PRIVATE
            ESSS CanarySupport
            ${LLVM_LINK_COMPONENTS}
            z ncurses pthread dl
    )
endif()

# Install the executable
install(TARGETS esss RUNTIME DESTINATION bin) 