Binary Decision Diagrams (BDDs)
===============================

This document describes the Binary Decision Diagram (BDD) capabilities in the Lotus framework.

Overview
--------

Lotus integrates the CUDD package to provide Binary Decision Diagram capabilities. BDDs are a data structure that is used to represent Boolean functions efficiently, useful for symbolic model checking and other verification tasks.

Features
--------

* Integration with the CUDD BDD package
* Support for various BDD operations:
  * Boolean operations (AND, OR, NOT, XOR)
  * Quantification (EXISTS, FORALL)
  * Variable substitution
  * Composition


Usage
-----

Basic BDD Operations
~~~~~~~~~~~~~~~~~~~~



Applications
------------

BDDs can be used for various applications in program analysis:

* **Symbolic Model Checking**: Verifying properties of finite state systems
* **Boolean Satisfiability Solving**: Finding assignments that satisfy Boolean formulas
* **Equivalence Checking**: Checking if two circuits or formulas are equivalent

Related Resources
-----------------

* `CUDD: Colorado University Decision Diagram Package <https://web.mit.edu/sage/export/tmp/y/usr/share/doc/polybori/cudd/cuddIntro.html>`_
* `Model Checking <https://mitpress.mit.edu/books/model-checking-second-edition>`_ (book) 