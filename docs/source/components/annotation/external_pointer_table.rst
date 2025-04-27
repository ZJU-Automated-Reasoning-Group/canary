External Pointer Table
====================

The External Pointer Table provides a mechanism for specifying pointer behavior of external functions that cannot be directly analyzed by Lotus.

Overview
--------

When analyzing programs that use external libraries, static analysis tools often lack visibility into the implementation of these libraries. This can lead to imprecise or incomplete analysis results. The External Pointer Table allows users to annotate external functions with information about:

- Memory allocation behavior
- Pointer copying between parameters and return values
- Memory escape tracking
- Functions that can be safely ignored

These annotations help improve the precision of pointer analysis by providing accurate models of external code behavior.

Configuration Format
-------------------

The External Pointer Table uses a custom configuration language to specify pointer effects. The basic syntax includes:

.. code-block:: text

   # Allocating functions
   malloc ALLOC
   calloc ALLOC Arg0
   
   # Copying functions (copy from Arg0 to return value)
   strcpy Arg1V Arg0D
   
   # Functions that can be ignored
   IGNORE printf

Annotation Types
---------------

The system supports several types of annotations:

**ALLOC** - Indicates a function allocates memory
  - Simple form: ``func ALLOC`` - Function allocates memory
  - With size argument: ``func ALLOC Arg0`` - Allocation size is in Arg0

**Copy Operations** - Specifies pointer values being copied
  - Position indicators:
    - ``Arg0``, ``Arg1``, etc. - Function arguments
    - ``Ret`` - Return value
  - Access types:
    - ``V`` - Value (direct pointer)
    - ``D`` - Direct memory (one level of dereference)
    - ``R`` - Reachable memory (all reachable memory)

**Special Sources** - Predefined pointer sources
  - ``NULL`` - Null pointer
  - ``UNKNOWN`` - Universal pointer (can point to anything)
  - ``STATIC`` - Static memory pointer

**IGNORE** - Functions to be excluded from analysis

Usage
-----

To use the External Pointer Table:

1. Create a configuration file with function annotations
2. Load the file using the ExternalPointerTable API:

.. code-block:: cpp

   // Load annotations from file
   auto fileContent = readFile("path/to/annotations.txt");
   auto pointerTable = ExternalPointerTable::buildTable(fileContent);
   
   // Look up pointer effect summary for a function
   if (auto summary = pointerTable.lookup("malloc")) {
       // Use the summary information in analysis
   }

Benefits
-------

- Improves analysis precision for code using external libraries
- Allows customization of analysis behavior for specific functions
- Enables modeling of complex memory behaviors that can't be automatically derived
- Supports incremental development of annotations as needed 