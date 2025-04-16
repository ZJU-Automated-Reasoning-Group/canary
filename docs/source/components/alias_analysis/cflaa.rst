CFLAA: Context-Free Language Reachability-Based Alias Analysis
==============================================================

This document describes the Context-Free Language Reachability-based Alias Analysis (CFLAA) component in the Lotus framework.

Overview
--------

CFLAA is an alias analysis technique that formulates the alias analysis problem as a context-free language (CFL) reachability problem. 


Implementation
--------------

The CFLAA implementation in Lotus is based on LLVM's original implementation, which was available in LLVM 14 and earlier versions. The files were copied from LLVM 14.0.6 since they were removed in later versions of LLVM.

CFL Reachability Formulation
----------------------------

The alias analysis problem is formulated as a CFL reachability problem as follows:

1. The program is represented as a labeled directed graph where:
   * Nodes represent memory objects or pointers
   * Edges represent pointer operations (assignments, loads, stores, etc.)

2. Edges are labeled to distinguish different kinds of relations:
   * Assignment edges (direct pointer assignments)
   * Dereference edges (loads and stores)
   * Call and return edges (for function calls and returns)

3. Aliases are determined by checking if there is a path between two nodes such that the concatenation of the edge labels forms a word in a specific context-free language.

