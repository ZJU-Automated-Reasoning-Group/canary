ESSS: Error Checking Bug Detector
=================================

This document describes the ESSS component in the Lotus framework.

Overview
--------

ESSS is a static analyzer for detecting error checking bugs in C/C++ programs. It focuses on identifying cases where error conditions are not properly handled, which can lead to security vulnerabilities.

Error Handling Patterns
-----------------------

ESSS can detect various error handling patterns and their misuse:

1. **Return Code Checking**: Functions often return a special value (e.g., NULL, -1) to indicate an error. Failing to check these return values can lead to bugs.

2. **Errno Checking**: Many standard library functions set the global `errno` variable when an error occurs. Failing to check `errno` after calling such functions can lead to bugs.

3. **Exception Handling**: In C++, exceptions are used for error handling. Failing to catch and handle exceptions properly can lead to bugs.

4. **Error Propagation**: Errors should be propagated to callers when they cannot be handled locally. Failing to propagate errors can hide bugs.

Bug Types
---------

Return Code Ignored
~~~~~~~~~~~~~~~~~~~

This bug occurs when a function's return value, which may indicate an error, is ignored.

Example:

.. code-block:: c

   // Bad: Return value ignored
   malloc(size);
   
   // Good: Return value checked
   void *ptr = malloc(size);
   if (ptr == NULL) {
       // Handle error
   }

Errno Not Checked
~~~~~~~~~~~~~~~~~

This bug occurs when a function that may set `errno` is called, but `errno` is not checked afterward.

Example:

.. code-block:: c

   // Bad: errno not checked
   float result = sqrt(value);
   
   // Good: errno checked
   errno = 0;
   float result = sqrt(value);
   if (errno != 0) {
       // Handle error
   }

Resource Leak on Error
~~~~~~~~~~~~~~~~~~~~~~

This bug occurs when a resource is acquired but not released when an error occurs.

Example:

.. code-block:: c

   // Bad: Resource leak on error
   FILE *file = fopen("file.txt", "r");
   if (file == NULL) {
       return -1;  // Resource leak if malloc fails
   }
   
   void *buffer = malloc(size);
   if (buffer == NULL) {
       return -1;  // File handle leaks
   }
   
   // Good: Resource properly released on error
   FILE *file = fopen("file.txt", "r");
   if (file == NULL) {
       return -1;
   }
   
   void *buffer = malloc(size);
   if (buffer == NULL) {
       fclose(file);  // Release file handle
       return -1;
   }

Usage
-----

Command-line Usage
~~~~~~~~~~~~~~~~~~

To use ESSS with the Lotus framework:

.. code-block:: bash

   # Run ESSS analysis
   ./esss input.bc


Implementation Details
----------------------

The ESSS component is implemented in the ``lib/Checker/ESSS`` directory. It uses static analysis techniques to track error states and error handling across functions.

