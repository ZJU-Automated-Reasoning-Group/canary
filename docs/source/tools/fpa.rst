FPA: Function Pointer Analysis
==========================

FPA is a function pointer analysis module that provides various algorithms to analyze indirect calls in programs. It helps determine the possible targets of function pointers by employing different type-based and data-flow analysis techniques.

Overview
--------

The FPA module includes several analysis algorithms:

- **FLTA (First Layer Type Analysis)**: Basic type-based analysis for function pointers
- **MLTA (Multi-Layer Type Analysis)**: Enhanced type analysis that considers multiple layers of type information
- **MLTADF (MLTA with Data Flow)**: MLTA augmented with data flow analysis
- **KELP**: A function pointer analysis technique that identifies simple function pointers with special handling

Usage
-----

.. code-block:: bash

   # Basic usage
   ./fpa [options] <input bitcode files>

   # Using FLTA analysis
   ./fpa -analysis-type=1 input.bc
   
   # Using MLTA analysis
   ./fpa -analysis-type=2 input.bc
   
   # Using MLTA with data flow analysis
   ./fpa -analysis-type=3 input.bc
   
   # Using KELP analysis
   ./fpa -analysis-type=4 input.bc

Options
-------

- ``-analysis-type=<N>``: Select analysis algorithm (1=FLTA, 2=MLTA, 3=MLTADF, 4=KELP)
- ``-max-type-layer=<N>``: Set maximum type layer for MLTA analysis (default: 10)
- ``-debug``: Enable debug output
- ``-output-file=<path>``: Output file path for results (use "cout" for standard output)

Output Format
------------

When an output file is specified, FPA produces results in the following format:

.. code-block:: text

   file.c:line:column|target1,target2,target3

Each line represents an indirect call site with its source location and the list of possible target functions.

Statistics
---------

FPA also outputs various statistics about the analysis, including:

- Number of virtual calls
- Number of indirect calls (and those with resolved targets)
- Number of indirect call targets
- Number of address-taken functions
- Number of multi-layer calls and targets
- Number of one-layer calls and targets
- Number of simple indirect calls
- Number of confined functions

Implementation Notes
-------------------

The FPA module builds upon and improves the Multi-Layer Type Analysis (MLTA) approach with several enhancements:

1. Type matching is performed by comparing types directly rather than function signatures
2. Special handling for escaped types and casting operations
3. Implementation of data flow analysis for improved precision
4. KELP implementation for identifying simple function pointers

The analysis handles various C/C++ constructs including function pointers in structures, global variables, bitcasts, and more. 