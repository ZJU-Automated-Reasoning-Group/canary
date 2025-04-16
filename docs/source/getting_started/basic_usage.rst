Basic Usage
===========

This guide covers the basic usage of Lotus for program analysis.

Getting Started
---------------

After installing Lotus following the :doc:`installation` guide, you can start using its tools and libraries for program analysis.

Command-line Tools
------------------

Lotus provides several command-line tools for different analysis tasks:

Alias Analysis
~~~~~~~~~~~~~~

To analyze pointer aliases using Sea-DSA:

.. code-block:: bash

    seadsa-tool input.bc

For Dyck-language based alias analysis:

.. code-block:: bash

    canary input.bc

Bug Finding
~~~~~~~~~~~

To detect integer overflow bugs using Kint:

.. code-block:: bash

    kint input.bc

To find error-handling bugs:

.. code-block:: bash

    esss input.bc

Working with LLVM Bitcode
-------------------------

Generating Bitcode
~~~~~~~~~~~~~~~~~~

Before using Lotus tools, you need to compile your C/C++ source code to LLVM bitcode:

.. code-block:: bash

    clang -emit-llvm -c -g example.c -o example.bc

For larger projects with multiple files:

.. code-block:: bash

    # Compile each source file to bitcode
    clang -emit-llvm -c -g source1.c -o source1.bc
    clang -emit-llvm -c -g source2.c -o source2.bc
    
    # Link bitcode files
    llvm-link source1.bc source2.bc -o linked.bc

Library Usage
-------------

Lotus can also be used as a library in your LLVM-based tools:

.. code-block:: cpp

    #include "lotus/PointerAnalysis.h"
    #include "lotus/BugFinder.h"
    
    // Use Lotus components in your LLVM pass
    
Example Workflow
----------------

A typical workflow for using Lotus might include:

1. Compile source code to LLVM bitcode
2. Run pointer analysis to build alias information
3. Perform bug detection based on the alias analysis results
4. Review and analyze the reported issues

Next Steps
----------

For more detailed usage of specific components, refer to the component-specific documentation:

* :doc:`/components/alias_analysis/index`
* :doc:`/components/bug_finding/index`
* :doc:`/components/constraint_solving/index` 