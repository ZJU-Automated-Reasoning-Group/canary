Project Structure
=================

This document provides an overview of the Lotus project structure to help you navigate the codebase.

Directory Structure
-------------------

The Lotus project is organized into the following main directories:

* ``include/``: Header files
* ``lib/``: Implementation files
* ``tools/``: Command-line tools
* ``tests/``: Test cases
* ``examples/``: Example programs
* ``docs/``: Documentation

Header Files (include/)
~~~~~~~~~~~~~~~~~~~~~~~

The ``include/`` directory contains the headers for various components of Lotus:

* ``include/Alias/``: Headers for alias analysis implementations (DyckAA, CFLAA, Sea-DSA, Andersen)
* ``include/Solvers/``: Headers for constraint solvers (SMT, BDD, WPDS)
* ``include/rapidjson/``: JSON parsing library headers

Implementation Files (lib/)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``lib/`` directory contains the implementation of the different components:

* ``lib/Alias/``: Alias analysis implementations
  
  * ``lib/Alias/DyckAA/``: Dyck-language-based alias analysis
  * ``lib/Alias/CFLAA/``: Context-free language reachability-based alias analysis
  * ``lib/Alias/seadsa/``: Sea-DSA context-sensitive points-to analysis
  * ``lib/Alias/Andersen/``: Andersen's points-to analysis

* ``lib/Solvers/``: Constraint solving implementations
  
  * ``lib/Solvers/SMT/``: SMT solver (Z3 integration)
  * ``lib/Solvers/CUDD/``: BDD implementation using CUDD
  * ``lib/Solvers/WPDS/``: Weighted pushdown system library

* ``lib/Checker/``: Bug finding implementations
  
  * ``lib/Checker/Kint/``: Integer-related bug detector

* ``lib/Transform/``: IR transformation passes

Command-line Tools (tools/)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``tools/`` directory contains the entry points for various command-line utilities:

* ``tools/lotustool/``: Main tool for running analyses
* ``tools/kint/``: Bug detector tool
* ``tools/csr/``: Context-sensitive reachability tool
* ``tools/seadsa-tool/``: Sea-DSA analysis tool
* ``tools/owl/``: SMT solver interface


Examples (examples/)
~~~~~~~~~~~~~~~~~~~~

The ``examples/`` directory contains example programs to demonstrate the capabilities of Lotus.

Build System
------------

Lotus uses CMake for building. The main configuration file is ``CMakeLists.txt`` in the project root. Additional configuration files may be found in subdirectories.

Key build options include:

* ``-DLLVM_BUILD_PATH``: Path to LLVM build directory
* ``-DCUSTOM_BOOST_ROOT``: (Optional) Path to custom Boost installation

Development Environment
-----------------------

For development, we recommend using an IDE or editor with C++ support such as:

* Visual Studio Code with C/C++ extensions
* CLion
* Vim/Emacs with C++ plugins

The project includes configuration files for these environments in the repository. 