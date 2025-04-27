Lotus Components
================

This section provides detailed information about the major components within the Lotus framework.

.. toctree::
   :maxdepth: 2

   alias_analysis/index
   constraint_solving/index
   bug_finding/index
   utilities/index
   annotation/index

Alias Analysis
--------------

- :doc:`DyckAA <alias_analysis/dyckaa>`: A unification-based, exhaustive alias analysis
- :doc:`CFLAA <alias_analysis/cflaa>`: Context-Free Language Reachability-based Alias Analysis
- :doc:`Sea-DSA <alias_analysis/sea_dsa>`: A context-sensitive, field-sensitive alias analysis based on Data Structure Analysis
- :doc:`Andersen <alias_analysis/andersen>`: Context-insensitive points-to analysis implementation
- :doc:`Dynamic <alias_analysis/dynamic>`: Runtime-based alias analysis that uses program execution data

Constraint Solving
------------------

- :doc:`SMT Solver <constraint_solving/smt>`: Z3 integration for SMT solving
- :doc:`BDD <constraint_solving/bdd>`: Binary Decision Diagram with CUDD
- :doc:`WPDS <constraint_solving/wpds>`: Weighted Pushdown System library

Bug Finding
-----------

- :doc:`Kint <bug_finding/kint>`: Integer bug detector

Annotation System
----------------

- :doc:`External Pointer Table <annotation/external_pointer_table>`: Provides pointer behavior information for external functions
- :doc:`External ModRef Table <annotation/external_modref_table>`: Provides memory read/write behavior information for external functions

Utilities
---------

- :doc:`NullPointer <utilities/null_pointer>`: Null pointer analysis
- :doc:`RapidJSON <utilities/rapidjson>`: JSON parser/generator integration
- :doc:`Transform <utilities/transform>`: LLVM Bitcode transformations 