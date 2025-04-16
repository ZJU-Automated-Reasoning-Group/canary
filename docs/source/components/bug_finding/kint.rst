Kint: Integer Bug Detector
==========================

This document describes the Kint integer bug detector component in the Lotus framework.

Overview
--------

Kint is a static analyzer for detecting integer-related bugs in C/C++ programs. 

Features
--------

* Detection of various integer-related bugs:
  * Integer overflow/underflow
  * Division by zero
  * Bad shift (excessive shift amounts)

Bug Types
---------

Integer Overflow
~~~~~~~~~~~~~~~~

Integer overflow occurs when an arithmetic operation produces a result that exceeds the maximum value that can be represented by the integer type. For example, adding two positive integers may result in a negative number due to overflow.

Example:

.. code-block:: c

   int a = INT_MAX;
   int b = a + 1;  // Integer overflow: b becomes INT_MIN

Division by Zero
~~~~~~~~~~~~~~~~

Division by zero occurs when a program attempts to divide a number by zero, which is undefined behavior in C/C++.

Example:

.. code-block:: c

   int a = 10;
   int b = 0;
   int c = a / b;  // Division by zero

Bad Shift
~~~~~~~~~

Bad shift occurs when a shift operation uses an amount that is negative or exceeds the bit width of the operand. This can lead to undefined behavior.

Example:

.. code-block:: c

   int a = 1;
   int b = a << 32;  // Bad shift: shifting a 32-bit integer by 32 or more bits

Array Out of Bounds
~~~~~~~~~~~~~~~~~~~

Array out of bounds occurs when a program accesses an array element outside the bounds of the array. This can lead to memory corruption or information disclosure.

Example:

.. code-block:: c

   int arr[10];
   int i = 10;
   int x = arr[i];  // Array out of bounds access


Usage
-----

Command-line Usage
~~~~~~~~~~~~~~~~~~

To use Kint with the Lotus framework:

.. code-block:: bash

   # Enable all checkers
   ./kint input.ll --check-all=true

   # Enable specific checkers
   ./kint input.ll --check-int-overflow=true --check-div-by-zero=true

   # Set logging level
   ./kint input.ll --check-all=true --log-level=warning

Available Options
~~~~~~~~~~~~~~~~~

* ``--check-int-overflow=true|false``: Enable/disable integer overflow checking
* ``--check-div-by-zero=true|false``: Enable/disable division by zero checking
* ``--check-bad-shift=true|false``: Enable/disable bad shift checking
* ``--check-array-oob=true|false``: Enable/disable array out of bounds checking
* ``--check-dead-branch=true|false``: Enable/disable dead branch checking
* ``--check-all=true|false``: Enable/disable all checkers
* ``--log-level=debug|info|warning|error|none``: Set the log level
* ``--log-to-stderr``: Send logs to stderr instead of stdout
* ``--log-to-file=<filename>``: Send logs to a file
* ``--quiet``: Suppress non-error messages


Related Resources
-----------------

* `KINT: Integer Bug Finding <https://www.usenix.org/conference/usenixsecurity12/technical-sessions/presentation/wang-xi>`_ (paper)
* `Common Weakness Enumeration (CWE) <https://cwe.mitre.org/>`_:
  
  * `CWE-190: Integer Overflow or Wraparound <https://cwe.mitre.org/data/definitions/190.html>`_
  * `CWE-369: Divide By Zero <https://cwe.mitre.org/data/definitions/369.html>`_
  * `CWE-128: Wrap-around Error <https://cwe.mitre.org/data/definitions/128.html>`_
  * `CWE-190: Integer Overflow or Wraparound <https://cwe.mitre.org/data/definitions/190.html>`_ 