Sea-DSA: Data Structure Analysis
================================

This document describes the Sea-DSA (Data Structure Analysis) component in the Lotus framework.

Overview
--------

Sea-DSA is a context-sensitive, field-sensitive points-to analysis based on Data Structure Analysis (DSA). 


DSA Approach
------------

Data Structure Analysis (DSA) is a points-to analysis technique that builds memory graphs to represent the memory objects in a program and their relationships. The key aspects of DSA are:

1. **Nodes and Fields**: Each node in the memory graph represents a memory object, and fields represent different offsets within objects.

2. **Context-Sensitivity**: The analysis creates a separate memory graph for each calling context, which improves precision.

3. **Field-Sensitivity**: The analysis tracks different fields of data structures separately, which enables precise analysis of complex data structures.

4. **Collapsing**: When the analysis cannot precisely track a memory region (e.g., due to arbitrary pointer arithmetic), it conservatively collapses the corresponding node in the memory graph.

5. **Graph Operations**: The analysis defines operations on memory graphs, such as unification, to model program statements.

Usage
-----

Using Sea-DSA Tools
~~~~~~~~~~~~~~~~~~~

Lotus provides two command-line tools for using Sea-DSA:

* **seadsa-dg**: A tool for generating memory graphs
* **seadsa-tool**: A more general tool with various Sea-DSA analyses

.. code-block:: bash

   # Generate memory graphs
   ./seadsa-dg --sea-dsa-dot <input bitcode file>

   # Run Sea-DSA analysis
   ./seadsa-tool --sea-dsa-dot --outdir=<output directory> <input bitcode file>

API Usage
~~~~~~~~~

TBD



Related Resources
-----------------



