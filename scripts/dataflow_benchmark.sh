#!/bin/bash
#
# Benchmark script for dataflow analyses

set -e  # Exit immediately if a command fails

# Check if dataflow-tool exists
if [ ! -f "build/bin/dataflow-tool" ]; then
    echo "Error: dataflow-tool not found. Please build the project first."
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p results

# Path setup - script is now in script directory
SCRIPT_DIR="$(dirname "$(realpath "$0")")"
SPEC_DIR="$(dirname "${SCRIPT_DIR}")/benchmarks/spec2006"

# Small benchmarks to test with (these are quicker to analyze)
SMALL_BENCHMARKS=(
    "429.mcf.bc"
    "470.lbm.bc"
    "462.libquantum.bc"
    "401.bzip2.bc"
)

# Function to run analysis on a benchmark
run_analysis() {
    local benchmark=$1
    local analysis=$2
    local verbose=$3
    local time_flag=$4
    
    echo "Running $analysis analysis on ${benchmark}..."
    
    output_file="results/$(basename ${benchmark%.bc})_${analysis}.txt"
    
    # Command flags
    verbose_flag=""
    [ "$verbose" = "true" ] && verbose_flag="-v"
    
    time_option=""
    [ "$time_flag" = "true" ] && time_option="-time"
    
    # Run the analysis
    build/bin/dataflow-tool "${SPEC_DIR}/${benchmark}" -analysis ${analysis} ${verbose_flag} ${time_option} -o ${output_file}
}

# Parse command-line arguments
analysis="all"
verbose="false"
time_analysis="true"
use_all_benchmarks="false"

while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --analysis) analysis="$2"; shift; shift ;;
        --verbose) verbose="true"; shift ;;
        --all-benchmarks) use_all_benchmarks="true"; shift ;;
        --no-time) time_analysis="false"; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# Select benchmarks
benchmarks=("${SMALL_BENCHMARKS[@]}")
if [ "$use_all_benchmarks" = "true" ]; then
    benchmarks=()
    for file in "${SPEC_DIR}"/*.bc; do
        [ -f "$file" ] && benchmarks+=("$(basename ${file})")
    done
fi

# Print header
echo "==================================================================="
echo "Dataflow Analysis Benchmark - Analysis: ${analysis}, Benchmarks: ${#benchmarks[@]}"
echo "==================================================================="

# Record start time for the entire benchmark
start_time=$(date +%s)

# Run analyses
for benchmark in "${benchmarks[@]}"; do
    echo "Analyzing ${benchmark}"
    run_analysis "${benchmark}" "${analysis}" "${verbose}" "${time_analysis}"
done

# Calculate total time
end_time=$(date +%s)
total_time=$((end_time - start_time))

echo "Benchmark completed in ${total_time} seconds"

# Create summary file
summary_file="results/summary_${analysis}.txt"
echo "Dataflow Analysis Benchmark Summary" > ${summary_file}
echo "Analysis: ${analysis}" >> ${summary_file}
echo "Date: $(date)" >> ${summary_file}
echo "Total benchmarks: ${#benchmarks[@]}" >> ${summary_file}
echo "Total time: ${total_time} seconds" >> ${summary_file}

# Extract timing information if available
if [ "$time_analysis" = "true" ]; then
    echo "Performance Summary:" >> ${summary_file}
    for benchmark in "${benchmarks[@]}"; do
        result_file="results/$(basename ${benchmark%.bc})_${analysis}.txt"
        if [ -f "${result_file}" ]; then
            echo "- ${benchmark}:" >> ${summary_file}
            grep "completed in" ${result_file} >> ${summary_file}
        fi
    done
fi

echo "Summary written to ${summary_file}" 