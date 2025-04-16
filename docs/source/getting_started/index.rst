Getting Started with Lotus
==========================

This section provides guides on how to install and start using the Lotus framework for program analysis.

.. toctree::
   :maxdepth: 1

   installation
   basic_usage
   project_structure

Quick Installation
------------------

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

For more detailed installation instructions, including prerequisites and troubleshooting, see the :doc:`Installation Guide <installation>`. 