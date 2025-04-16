Development Roadmap
===================

This document outlines the planned development roadmap for the Lotus project. The roadmap is divided into several categories based on priority and type of feature.

Testing Improvements
--------------------

* **Unit Testing**: Add comprehensive unit tests, with priority on the SMT library components.
* **Integration Testing**: Develop integration tests to verify interaction between different Lotus components.
* **Test Coverage**: Improve test coverage across the codebase.

High Priority Features
----------------------

Pre-condition Inference Engine
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Integrate the pre-condition inference engine from Beacon (SP'22)
* This component relies on code from KLEE (not using the symbolic execution engine) and SVF

SVF Integration
~~~~~~~~~~~~~~~

* Integrate SVF (Static Value-Flow Analysis) as a library
* Implement various pointer analyses with different precision levels

IR Optimization
~~~~~~~~~~~~~~~

* Redundant load/store elimination
* Control-flow refinement
* Superoptimization techniques
* Dead code elimination

Program Slicing
~~~~~~~~~~~~~~~

* Conventional slicing
* Thin slicing
* Hybrid thin slicing
* Dynamic slicing

Instrumentation
~~~~~~~~~~~~~~~

* Guided fuzzing instrumentation
* Profiling instrumentation
* Coverage analysis
* Dynamic analysis support

Lower Priority Features
-----------------------

Symbolic Execution
~~~~~~~~~~~~~~~~~~

* Integrate symbolic emulation/execution capabilities
* Possible integration with KLEE or similar tools

Numerical Abstract Interpretation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Integrate abstract interpretation frameworks (e.g., IKOS, CLAM/Crab)
* Implement numerical domains (intervals, octagons, polyhedra)

Software Model Checking
~~~~~~~~~~~~~~~~~~~~~~~

* Integrate software model checking capabilities
* Possible integration with Smarck, Seahorn, or similar tools

Binary Analysis
~~~~~~~~~~~~~~~

* Support for analyzing IR lifted from binaries
* Integration with tools like Remill, retdec

Clang AST-based Analysis
~~~~~~~~~~~~~~~~~~~~~~~~

* Integrate Clang static analyzer or similar tools
* Implement bug checking at the AST level

Data Flow Analysis Framework
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Integrate a comprehensive data flow analysis framework
* Possible integration with Pharsar or similar tools

Timeline
--------

The timeline for these features is flexible and will depend on contributor availability and project priorities. High priority features are targeted for completion within the next 6-12 months, while lower priority features may take longer.

How to Contribute
-----------------

If you'd like to contribute to any of these roadmap items, please refer to the :doc:`Contributing Guide <contributing>` for information on how to get started. 