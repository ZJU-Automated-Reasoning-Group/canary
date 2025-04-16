Building Lotus
==============

This document provides detailed instructions for building Lotus from source.

Prerequisites
-------------

Make sure you have the following dependencies installed:

* CMake 3.13 or newer
* C++ compiler with C++14 support (GCC 7+, Clang 6+, MSVC 19.14+)
* LLVM 12.0.0 or newer
* Boost 1.65.0 or newer
* Z3 4.8.0 or newer

For detailed installation instructions for these dependencies, see the :doc:`/getting_started/installation` guide.

Build Configuration
-------------------

Standard Build
~~~~~~~~~~~~~~

To build Lotus with default options:

.. code-block:: bash

    mkdir build && cd build
    cmake ..
    make -j$(nproc)  # On Linux/macOS
    # or
    cmake --build . --config Release -j  # Platform-independent

Build Types
~~~~~~~~~~~

Lotus supports several build types:

* Debug: ``-DCMAKE_BUILD_TYPE=Debug``
* Release: ``-DCMAKE_BUILD_TYPE=Release``
* RelWithDebInfo: ``-DCMAKE_BUILD_TYPE=RelWithDebInfo``

Example:

.. code-block:: bash

    cmake -DCMAKE_BUILD_TYPE=Debug ..

Build Options
-------------

The following CMake options can be used to customize the build:

``-DLOTUS_ENABLE_TESTS=ON|OFF``
  Enable or disable building tests (default: ON)

``-DLOTUS_BUILD_DOCS=ON|OFF``
  Enable or disable building documentation (default: OFF)

``-DLOTUS_USE_SYSTEM_Z3=ON|OFF``
  Use system-installed Z3 instead of building from source (default: ON)

``-DLLVM_DIR=<path>``
  Path to LLVM CMake configuration directory

Example:

.. code-block:: bash

    cmake -DLOTUS_ENABLE_TESTS=ON -DLOTUS_BUILD_DOCS=ON -DLLVM_DIR=/usr/lib/llvm-12/cmake ..

Installing
----------

To install Lotus to the default location:

.. code-block:: bash

    make install

To specify a custom installation directory:

.. code-block:: bash

    cmake -DCMAKE_INSTALL_PREFIX=/your/install/path ..
    make install

Troubleshooting
---------------

Common Build Issues
~~~~~~~~~~~~~~~~~~~

* **CMake can't find LLVM**: Set the ``LLVM_DIR`` variable to the LLVM CMake configuration directory
* **Missing Boost components**: Install the required Boost libraries or set ``BOOST_ROOT``
* **Z3 not found**: Install Z3 or set ``Z3_DIR`` to point to the Z3 CMake configuration 