Welcome to Lotus's Documentation!
=================================

Introduction
============

Lotus is a program analysis, verification, and optimization framework. It provides several toolkits that can be used
individually or in combination to perform different tasks on LLVM IR.

The current version has been tested on x86 Linux and ARM Mac using LLVM-12 and LLVM-14 with Z3-4.11.

Major Components
================

Alias Analysis
--------------

* **DyckAA**: A unification-based, exhaustive alias analysis (``lib/Alias/DyckAA``)
* **CFLAA**: Context-Free Language Reachability-based Alias Analysis (``lib/Alias/CFLAA``)
* **Sea-DSA**: A context-sensitive, field-sensitive alias analysis based on Data Structure Analysis (``lib/Alias/seadsa``)
* **Andersen**: Context-insensitive points-to analysis implementation (``lib/Alias/Andersen``)

Constraint Solving
------------------

* **SMT Solving**: Z3 integration (``lib/Solvers/SMT``)
* **Binary Decision Diagram (BDD)**: CUDD-based implementation (``lib/Solvers/CUDD``)
* **WPDS**: Weighted Pushdown System library for program analysis (``lib/Solvers/WPDS``)

Bug Finding
-----------

* **Kint**: A static bug finder for integer-related and taint-style bugs
* **ESSS**: For finding error checking bugs

Utilities
---------

* **NullPointer**: Null pointer analysis
* **RapidJSON**: A fast JSON parser/generator for C++ with both SAX/DOM style API (``include/rapidjson``)
* **Transform**: Transformations for LLVM Bitcode

Installing and Using Lotus
==========================

Install Lotus from source
-------------------------

::

  git clone https://github.com/ZJU-Automated-Reasoning-Group/lotus
  cd lotus
  mkdir build && cd build
  cmake ../ -DLLVM_BUILD_PATH=/path/to/llvm/build
  make -j$(nproc)

The setup script will:
- Configure the build with CMake
- Build the project using available CPU cores
- Generate executables for various analysis tools

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   getting_started/index
   components/index
   tools/index
   examples/index
   dev_guide/index 