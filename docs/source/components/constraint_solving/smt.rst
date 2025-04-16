SMT Solving
===========

This document describes the SMT (Satisfiability Modulo Theories) solving capabilities in the Lotus framework.

Overview
--------

Lotus integrates the Z3 SMT solver to provide powerful constraint solving capabilities. SMT solvers are used to determine whether a formula in first-order logic is satisfiable with respect to various theories like arithmetic, arrays, and bit-vectors.


Usage
-----

Using the Owl SMT Solver Interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Lotus provides a command-line interface called ``owl`` for working with SMT formulas:

.. code-block:: bash

   # Solve an SMT-LIB2 file
   ./owl file.smt2

   # Check satisfiability
   ./owl --check-sat file.smt2

   # Generate a model
   ./owl --model file.smt2

Programmatic Usage
~~~~~~~~~~~~~~~~~~

The SMT solver can also be used programmatically through the Lotus C++ API:

(TBD)



Related Resources
-----------------

* `Z3 Theorem Prover <https://github.com/Z3Prover/z3>`_
* `SMT-LIB <http://smtlib.cs.uiowa.edu/>`_
* `Decision Procedures <https://link.springer.com/book/10.1007/978-3-540-74105-3>`_ (book) 