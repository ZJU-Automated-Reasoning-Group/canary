Andersen's Points-to Analysis
=============================

This document describes the Andersen's points-to analysis component in the Lotus framework.

Overview
--------

Andersen's analysis is a context-insensitive, flow-insensitive points-to analysis that solves a system of set inclusion constraints derived from a program's pointer-manipulating statements. It is one of the most fundamental points-to analyses and forms the basis for many more sophisticated analyses.

Features
--------

* Field-sensitive analysis (distinguishes different struct fields)
* Inclusion-based constraint resolution
* Efficient constraint graph implementation
* Support for external library modeling

Constraint Generation
---------------------

Andersen's analysis generates inclusion constraints from the program's pointer operations:

1. **Address-of**: ``p = &q`` generates ``{q} ⊆ pts(p)``
2. **Assignment**: ``p = q`` generates ``pts(q) ⊆ pts(p)``
3. **Load**: ``p = *q`` generates ``∀r ∈ pts(q): pts(r) ⊆ pts(p)``
4. **Store**: ``*p = q`` generates ``∀r ∈ pts(p): pts(q) ⊆ pts(r)``

These constraints form a graph where nodes represent pointers and edges represent subset relationships. The analysis resolves these constraints iteratively until a fixed point is reached.

Usage
-----

Using Andersen's Analysis
~~~~~~~~~~~~~~~~~~~~~~~~~

To use Andersen's analysis with the Lotus framework:

.. code-block:: bash

   TBD

API Usage
~~~~~~~~~

TBD


Implementation Details
----------------------

Andersen's analysis is implemented in the ``lib/Alias/Andersen`` directory.

Key Files
~~~~~~~~~

* ``include/Alias/Andersen/Andersen.h``: Main analysis pass
* ``include/Alias/Andersen/ConstraintGraph.h``: Constraint graph representation
* ``lib/Alias/Andersen/Andersen.cpp``: Implementation of the analysis pass
* ``lib/Alias/Andersen/ConstraintGraph.cpp``: Implementation of constraint graph operations

Optimizations
-------------

The implementation includes several optimizations to improve the efficiency of the analysis:

* **Offline Variable Substitution**: Pre-processes the constraints to identify equivalent variables
* **Cycle Detection**: Identifies and collapses cycles in the constraint graph
* **Constraint Reduction**: Reduces the number of constraints through variable substitution
* **Incremental Updates**: Updates the points-to sets incrementally during constraint resolution

Applications
------------

Andersen's analysis can be used for various applications in program analysis:

* **Alias Analysis**: Determining which pointers may alias
* **Call Graph Construction**: Resolving virtual method calls and function pointers
* **Dead Code Elimination**: Identifying unreachable code
* **Escape Analysis**: Determining whether objects escape their defining scope

Related Resources
-----------------

* `Program Analysis and Specialization for the C Programming Language <https://dl.acm.org/doi/10.5555/2668133>`_ (Lars Ole Andersen's Ph.D. thesis)