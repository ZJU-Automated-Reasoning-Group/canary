DyckAA: Dyck-Language-Based Alias Analysis
==========================================

DyckAA is a unification-based, exhaustive alias analysis implemented in the Lotus framework. It uses Dyck-CFL reachability to resolve aliases in a context-sensitive manner.

Overview
--------

DyckAA is an alias analysis that models pointer operations using balanced parentheses (Dyck language). The balanced parentheses property allows the analysis to maintain context sensitivity while analyzing function calls and returns, leading to more precise results than context-insensitive approaches.

Features
--------

* Support LLVM IR (version 12, 14, etc?)
* Context-insensitive alias analysis
* Function pointer resolution

Usage
-----

DyckAA is implemented as an LLVM ModulePass and can be used with the LLVM pass manager. Here's how to use it with the Lotus framework:

.. code-block:: bash

   ./canary <input bitcode file>

Available Options
-----------------

DyckAA supports several command-line options to customize its behavior:

``-print-alias-set-info``
  
  Prints the evaluation of alias sets and outputs all alias sets and their relations (DOT format).

``-count-fp``
  
  Counts how many functions a function pointer may point to.

``-no-function-type-check``
  
  If set, disables function type checking when resolving pointer calls. Otherwise, only FuncTy-compatible functions can be aliased with a function pointer. Two functions f1 and f2 are FuncTy-compatible if:
  
  - Both or neither are variadic functions
  - Both or neither have a non-void return value
  - They have the same number of parameters
  - Parameters have the same FuncTy store sizes
  - There is an explicit cast operation between FuncTy(f1) and FuncTy(f2) (works with ``-with-function-cast-comb`` option)

``-dot-dyck-callgraph``
  
  Prints a call graph based on the alias analysis. Can be used with ``-with-labels`` option to add labels (call instructions) to the edges in call graphs.


Implementation Details
----------------------

DyckAA It maintains a graph of pointer relations and uses a unification-based approach to resolve aliases.

The core algorithm is based on the PLDI'13 paper `Fast Algorithms for Dyck-CFL-Reachability with Applications to Alias Analysis` by Qirun Zhang, Michael R. Lyu, Hao Yuan, and Zhendong Su.

Related Resources
-----------------

* `Dyck CFL Reachability <https://en.wikipedia.org/wiki/Context-free_language>`_