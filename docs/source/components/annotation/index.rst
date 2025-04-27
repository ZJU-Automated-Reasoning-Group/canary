Annotation System
===============

This section covers the annotation system components available in the Lotus framework.

.. toctree::
   :maxdepth: 1

   external_pointer_table
   external_modref_table

The Lotus annotation system provides mechanisms for specifying and using external information about program behavior, particularly for external library functions and complex code patterns that are difficult to analyze automatically.

Components
---------

The annotation system consists of several key components:

- :doc:`External Pointer Table <external_pointer_table>`: Provides pointer behavior information for external functions
- :doc:`External ModRef Table <external_modref_table>`: Provides memory read/write behavior information for external functions

These components allow analysts to supply additional information about code that cannot be directly analyzed, improving the precision and completeness of program analyses. 