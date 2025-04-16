Bug Finding Tools
=================

This document provides an overview of bug finding tools for C/C++ programs available in Lotus.

Tool Categories
---------------

Bug finding tools for C/C++ can be categorized based on their underlying technology and approach.

Kint Bug Detector Checkers
--------------------------

Kint is a static analyzer for detecting integer bugs in C/C++ programs. It uses LLVM IR as its input and can detect various types of integer-related bugs.

Available Checkers
~~~~~~~~~~~~~~~~~~

The following checkers are available in Kint:

1. **Integer Overflow** (``check-int-overflow``): Detects integer overflow conditions in arithmetic operations (addition, subtraction, multiplication, division).

2. **Division by Zero** (``check-div-by-zero``): Detects potential division by zero conditions.

3. **Bad Shift** (``check-bad-shift``): Detects shift operations with excessive shift amounts (e.g., shifting a 32-bit value by 32 or more bits).

4. **Array Out of Bounds** (``check-array-oob``): Detects array index out of bounds conditions when accessing arrays.

5. **Dead Branch** (``check-dead-branch``): Detects branches that cannot be taken due to contradictory conditions.

Since all checkers are disabled by default, you only need to enable the specific bug types you're interested in. This allows for targeted analysis and better performance when analyzing large codebases. 

Usage
~~~~~

By default, all checkers are disabled. You can enable checkers using command-line flags:

.. code-block:: bash

   # Enable all checkers at once
   ./kint input.ll --check-all=true

   # Enable specific checkers
   ./kint input.ll --check-int-overflow=true --check-div-by-zero=true

   # Enable only array out of bounds checking
   ./kint input.ll --check-array-oob=true

   # Enable all except dead branch detection
   ./kint input.ll --check-all=true --check-dead-branch=false

Logging Options
~~~~~~~~~~~~~~~

Kint provides several options to control the logging output:

You can set the verbosity of the log output using the ``--log-level`` option:

.. code-block:: bash

   # Show all messages including debug information
   ./kint input.ll --log-level=debug

   # Show informational messages and above (default)
   ./kint input.ll --log-level=info

   # Show only warnings and errors
   ./kint input.ll --log-level=warning

   # Show only errors
   ./kint input.ll --log-level=error

   # Suppress all log output
   ./kint input.ll --log-level=none

You can control where log messages are sent:

.. code-block:: bash

   # Send logs to stderr instead of stdout
   ./kint input.ll --log-to-stderr

   # Send logs to a file
   ./kint input.ll --log-to-file=kint.log

   # Suppress most log output (equivalent to --log-level=none)
   ./kint input.ll --quiet

ESSS Error Checking Bug Detector
--------------------------------

ESSS is a tool for finding error checking bugs in C/C++ programs. More details on this tool will be added soon. 