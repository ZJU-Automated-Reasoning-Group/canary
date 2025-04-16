Weighted Pushdown Systems (WPDS)
================================

This document describes the Weighted Pushdown System (WPDS) capabilities in the Lotus framework.

Overview
--------

A Weighted Pushdown System (WPDS) is a powerful formalism for modeling program behavior, particularly for analyzing interprocedural control flow. Lotus provides a WPDS library for program analysis tasks.

Features
--------

* Implementation of Weighted Pushdown Systems
* Support for various semirings (boolean, counting, bounded, etc.)
* Post* and pre* computations
* Witness generation for path extraction
* Handling of interprocedural control flow

Theory
------

A pushdown system is a transition system where the states are pairs consisting of a control location and a stack. The transitions can push or pop elements from the stack, or just change the control location.

A weighted pushdown system associates a weight with each transition from a given semiring. This allows for the modeling of various program properties, such as:

* Reachability analysis
* Dataflow analysis
* Security property verification
* Resource usage analysis

Usage
-----

Basic WPDS Operations
~~~~~~~~~~~~~~~~~~~~~

Here is an example of using the WPDS interface (To check):

(TBD)


Implementation Details
----------------------

The Lotus WPDS interface is implemented in the ``lib/Solvers/WPDS`` directory. 

Key Files
~~~~~~~~~

* ``include/Solvers/WPDS/WPDS.h``: Main interface for WPDS
* ``include/Solvers/WPDS/Semiring.h``: Semiring interface
* ``include/Solvers/WPDS/BooleanSemiring.h``: Boolean semiring implementation
* ``lib/Solvers/WPDS/WPDS.cpp``: Implementation of WPDS operations


Related Resources
-----------------

* `Weighted Pushdown Systems and Their Application to Interprocedural Dataflow Analysis <https://link.springer.com/chapter/10.1007/3-540-45126-6_12>`_ (paper)
* `Interprocedural Dataflow Analysis in the Presence of Pointers, Function Pointers, and Recursion <https://link.springer.com/chapter/10.1007/3-540-69053-0_23>`_ (paper)