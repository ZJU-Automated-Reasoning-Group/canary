External ModRef Table
==================

The External ModRef Table provides a mechanism for specifying memory read and write behavior (Mod/Ref) of external functions that cannot be directly analyzed by Lotus.

Overview
--------

Static analysis tools often lack visibility into the implementation of external library functions, which can lead to imprecise or incomplete analysis results when tracking memory modifications. The External ModRef Table allows users to annotate external functions with information about which memory locations they may read from (Ref) or write to (Mod).

These annotations are particularly important for:
- Library functions that modify memory through pointers
- System calls with side effects
- Functions that access global state
- Complex memory manipulation functions

Configuration Format
-------------------

The External ModRef Table uses a configuration language similar to the External Pointer Table. The basic syntax includes:

.. code-block:: text

   # Read-only function (references Arg0 but doesn't modify)
   strlen REF Arg0
   
   # Function that modifies memory (modifies memory pointed to by Arg0)
   memset MOD Arg0
   
   # Function that both reads and modifies
   strcpy REF Arg1 MOD Arg0
   
   # Functions that can be ignored
   IGNORE printf

Annotation Types
---------------

The system supports several types of annotations:

**REF** - Indicates a function reads from a memory location
  - ``REF Arg0`` - Function reads from memory pointed to by argument 0
  - ``REF Arg0D`` - Function reads from memory directly referenced by argument 0
  - ``REF Arg0R`` - Function reads from memory reachable from argument 0

**MOD** - Indicates a function modifies a memory location
  - ``MOD Arg0`` - Function modifies memory pointed to by argument 0
  - ``MOD Arg0D`` - Function modifies memory directly referenced by argument 0
  - ``MOD Arg0R`` - Function modifies memory reachable from argument 0

**IGNORE** - Functions to be excluded from analysis

Access types can be combined with position indicators:
- ``Arg0``, ``Arg1``, etc. - Function arguments
- ``Ret`` - Return value

Usage
-----

To use the External ModRef Table:

1. Create a configuration file with function annotations
2. Load the file using the ExternalModRefTable API:

.. code-block:: cpp

   // Load annotations from file
   auto fileContent = readFile("path/to/modref_annotations.txt");
   auto modrefTable = ExternalModRefTable::buildTable(fileContent);
   
   // Look up modref summary for a function
   if (auto summary = modrefTable.lookup("memcpy")) {
       if (summary->mods(arg0Position)) {
           // Function modifies memory at arg0
       }
       if (summary->refs(arg1Position)) {
           // Function reads memory at arg1
       }
   }

Benefits
-------

- Improves precision of alias analysis and data flow tracking
- Enables accurate modeling of memory side effects for external functions
- Supports alias analysis clients that need to know about memory dependencies
- Complements pointer behavior annotations to provide comprehensive external function modeling 