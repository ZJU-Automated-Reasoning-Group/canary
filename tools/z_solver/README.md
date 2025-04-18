# InvFinder

InvFinder is a powerful tool for program verification. It provides various algorithms for finding invariants in transition systems and supports different invariant domains.

## Features

- **Invariant Domains**:
  - Unsigned Bitvector Intervals (`UBVIntervalInvariant`)
  - Unsigned Bitvector Octagons (`UBVOctagonInvariant`)
  - Arbitrary Bitvector Bitwise (`ABVBitwiseInvariant`)

- **Synthesis Algorithms**:
  - OMT (Optimization Modulo Theories)
  - Binary Lifting
  - Decremental/Incremental approaches
  - Bilateral search

- **K-Induction Support**:
  - Automated k-induction for program verification

- **Input Format**:
  - CHC (Constrained Horn Clauses) in SMT2 format
  - C programs (via c2chc frontend)
  - SyGuS (Syntax-Guided Synthesis) specifications (via sygus2chc frontend)

## Prerequisites

- CMake (version 3.0 or higher)
- C++17 compatible compiler
- Z3 (tested on version 4.13.3 or higher)

## Building from Source

```bash

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make -j4
```

## Usage

### InvFinder Main Tool

The main `invFinder` tool can be used for invariant synthesis and k-induction verification:

```bash
# For invariant synthesis
./bin/bestInv -F <input_file> -O <output_csv> -D <domain> -M <method> [-T <timeout>]

# For k-induction verification
./bin/kInductor -F <input_file> -O <output_csv> -D <domain> -M <method> [-K <k_limit>] [-T <timeout>]
```

Parameters:
- `-F file`: Input CHC SMT2 file
- `-O output`: CSV file to write results
- `-D domain`: Invariant domain (`interval`, `octagon`, or `bitwise`) (`none` if using kInductor and want a raw k-induction)
- `-M method`: Algorithm to use (see below) (`none` if using kInductor and want a raw k-induction)
- `-T timeout`: Synthesis timeout in seconds (-1 for no timeout, default)
- `-K k-limit`: Maximum value of K for k-induction (default: 64)
- `-h`: Display help information and usage examples

Available methods:
- For interval domain: `binlift`, `decr`, `incr`, `omtfix`, `lenbs`, `optbin`, `binfix`, `bi`
- For octagon domain: `decr`, `optbin`
- For bitwise domain: `guess`, `bi`

Examples:
```bash
# Invariant synthesis example
./bin/bestInv -F data/nove/32lia/const_false-unreach-call1.sl_32bits_unsigned.smt2 -O results.csv -D interval -M optbin

# K-induction example
./bin/kInduction -F data/nove/32lia/const_false-unreach-call1.sl_32bits_unsigned.smt2 -O results.csv -D none -M none -K 10
```

### Frontend Tools

Convert C programs or SyGuS specifications to CHC format:

```bash
# Convert C to CHC
./bin/c2chc <input_c_file> <output_smt2_file>

# Convert SyGuS to CHC
./bin/sygus2chc <input_sygus_file> <output_smt2_file>
```

## Testing

Run the test suite:

```bash
cd build
ctest
```

Or run the test executable directly:

```bash
./bin/z_solver_tests
```

## Output Format

The bestInv tool generate CSV files with the following columns:
- File: Input file path
- Method: Algorithm used
- Bad Solve: Error status
- Time Cost: Execution time in seconds
- Calls: Number of solver calls (SMT + OMT)
- Info: Synthesized invariant

The kInductor tool generate CSV files with the following columns:
- File: Input file path
- Method: Algorithm used
- Proved: If there is a proof can be generated
- Result: True if proved safe, false otherwise
- K_needed: Number of minimum k for the proof(only useful when Proved=true)
- Time Cost: k-induction time in seconds

