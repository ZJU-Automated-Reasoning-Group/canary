#pragma once

// Configuration for MLTA (Multi-Layer Type Analysis)
enum MLTAMode {
    NoIndirectCalls,   // No indirect call analysis
    MatchSignatures,   // Consider all matching type signatures as possible call targets
    FullMLTA           // Full MLTA implementation
};

// Configuration switches for ESSS
namespace Config {
    // Debug levels
    constexpr unsigned ESSS_LOG_INFO = 1;
    constexpr unsigned ESSS_LOG_DEBUG = 2;
    constexpr unsigned ESSS_LOG_VERBOSE = 3;
    
    // Feature toggles
    constexpr bool ENABLE_DEBUG_OUTPUT = true;
    constexpr bool ENABLE_ERROR_PROPAGATION = true;
} 