Sea-DSA Tool
============

Overview
--------

The Sea-DSA tool is a command-line utility for running the Sea-DSA (Data Structure Analysis) on LLVM bitcode files. It provides a convenient interface to access the powerful alias analysis capabilities of Sea-DSA.

Features
--------

* Context-sensitive, field-sensitive pointer analysis
* Memory graph visualization
* Integration with other Lotus tools
* Stand-alone operation for alias analysis

Usage
-----

Basic Usage
~~~~~~~~~~~

To analyze a bitcode file with default options:

.. code-block:: bash

    seadsa-tool input.bc

Common Options
~~~~~~~~~~~~~~

``--mem-dot[=<file>]``
  Saves the memory graph in DOT format to the specified file or to standard output if no file is given

``--help``
  Displays all available options for the tool

``--inter``
  Uses inter-procedural (context-sensitive) analysis

``--intra``
  Uses intra-procedural (function-local) analysis

``--sea-dsa-stats``
  Shows statistics about the analysis results

Command Examples
----------------

Context-sensitive Analysis with Graph Output
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    seadsa-tool --inter --mem-dot=memory.dot input.bc

This runs Sea-DSA in inter-procedural mode and outputs the memory graph to memory.dot.

Analyzing a Function
~~~~~~~~~~~~~~~~~~~~

To analyze a specific function:

.. code-block:: bash

    seadsa-tool --intra --entry-point=main input.bc

This analyzes only the main function using intra-procedural analysis.

Programmatic Usage
------------------

Sea-DSA can also be used programmatically in LLVM passes:

.. code-block:: cpp

    #include "sea_dsa/Graph.hh"
    #include "sea_dsa/DsaAnalysis.hh"
    
    // Get the DSA graph for a function
    const sea_dsa::Graph& graph = sea_dsa::getDsaGraph(F); 