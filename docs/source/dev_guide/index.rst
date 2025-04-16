Development Guide
=================

This section provides information for developers who want to contribute to the Lotus project or extend its functionality.

.. toctree::
   :maxdepth: 1

   roadmap
   contributing
   testing
   building

Development Roadmap
-------------------

Testing
~~~~~~~

- Add unit tests (at least for the SMT library)

New Features
~~~~~~~~~~~~

- Integrate the pre-condition inference engine in Beacon (SP'22)
- Integrate SVF as a library for pointer analyses with different precisions
- IR optimization: redundant load/store elimination, control-flow refinement, superoptimization
- Program slicing (conventional, thin slicing, hybrid thin slicing)
- Instrumentation for guided fuzzing, profiling, etc.
- *(Low priority)* Symbolic emulation/execution
- *(Low priority)* Numerical abstract interpretation
- *(Low priority)* Software model checking
- *(Low priority)* Analyzing IR lifted from binaries
- *(Low priority)* Clang AST-based bug checking
- *(Low priority)* Data flow analysis framework

For more details on each planned feature, see the :doc:`Roadmap <roadmap>` document. 