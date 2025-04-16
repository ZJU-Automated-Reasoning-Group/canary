Installation Guide
==================

This guide walks you through the process of installing the Lotus framework on your system.

Prerequisites
-------------

Before installing Lotus, make sure you have the following prerequisites installed:

* LLVM 12.0.0 or 14.0.0
* Z3 4.11
* CMake 3.10+
* C++14 compatible compiler

Installing LLVM
---------------

Lotus requires LLVM to be built from source. Follow these steps to build LLVM:

.. code-block:: bash

   # Clone LLVM repository
   git clone https://github.com/llvm/llvm-project.git
   cd llvm-project

   # Checkout desired version
   git checkout llvmorg-14.0.0  # or llvmorg-12.0.0

   # Build LLVM
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ../llvm
   make -j$(nproc)  # Uses all available CPU cores

Installing Z3
-------------

Z3 is required for the SMT solving capabilities of Lotus. You can install it from source or use a package manager:

From Source
~~~~~~~~~~~

.. code-block:: bash

   git clone https://github.com/Z3Prover/z3.git
   cd z3
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make -j$(nproc)
   sudo make install

Using Package Manager
~~~~~~~~~~~~~~~~~~~~~

Ubuntu/Debian
^^^^^^^^^^^^^

.. code-block:: bash

   sudo apt-get install libz3-dev

macOS (using Homebrew)
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   brew install z3

Building Lotus
--------------

Once you have installed the prerequisites, you can build Lotus:

.. code-block:: bash

   # Clone the repository
   git clone https://github.com/ZJU-Automated-Reasoning-Group/lotus
   cd lotus

   # Create a build directory
   mkdir build && cd build

   # Configure with CMake
   cmake ../ -DLLVM_BUILD_PATH=/path/to/llvm/build

   # Build using all available cores
   make -j$(nproc)

Custom Boost Installation (Optional)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The build system will automatically download and build Boost if it's not found on your system. If you want to use a custom Boost installation, you can specify its path:

.. code-block:: bash

   cmake ../ -DLLVM_BUILD_PATH=/path/to/llvm/build -DCUSTOM_BOOST_ROOT=/path/to/boost

Verifying the Installation
--------------------------

After building, you can verify that the installation was successful by running one of the tools:

.. code-block:: bash

   # Check the version of the SMT solver
   ./owl --version

   # Run a simple test with DyckAA
   ./lotustool -load-pass-plugin=./libDyckAA.so -passes=dyck-aa -disable-output test.bc

Troubleshooting
---------------

Common Issues
~~~~~~~~~~~~~

1. **LLVM Version Mismatch**: Ensure that you are using the correct version of LLVM (12.0.0 or 14.0.0).

2. **Z3 Not Found**: Make sure Z3 is installed and the libraries are in your system's library path.

3. **Build Errors**: If you encounter build errors, check that you have all the required dependencies and that they are the correct versions.

For more help, please submit an issue on the `GitHub repository <https://github.com/ZJU-Automated-Reasoning-Group/lotus/issues>`_. 