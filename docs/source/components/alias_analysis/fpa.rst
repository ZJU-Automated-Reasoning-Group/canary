FPA (Function Pointer Analysis)
=============================

The Function Pointer Analysis (FPA) module provides specialized analyses for determining possible targets of indirect function calls. It implements several algorithms with different precision and performance characteristics.

Implementation
-------------

The FPA module is located in ``lib/Alias/FPA`` and provides the following key components:

Analysis Classes
^^^^^^^^^^^^^^^

- **FLTAPass**: First-Layer Type Analysis that uses basic type matching for resolving function pointers
- **MLTAPass**: Multi-Layer Type Analysis that considers nested types and structural information
- **MLTADFPass**: Data-flow enhanced version of MLTA that improves precision through data flow tracking
- **KELPPass**: An analysis focused on "simple function pointers" with special handling rules

Key Data Structures
^^^^^^^^^^^^^^^^^^

- **typeEscapedSet**: Tracks which types have escaped the type system through casts or other means
- **typeCapSet**: Records type information for variables to constrain possible targets
- **Callees**: Maps indirect call sites to their possible target functions

Algorithms
----------

FLTA (First-Layer Type Analysis)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The FLTA algorithm performs basic type-based function pointer resolution:

1. Identifies all address-taken functions in the module
2. For each indirect call, matches the call signature with compatible functions
3. Returns the set of type-compatible functions as possible targets

MLTA (Multi-Layer Type Analysis)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

MLTA enhances FLTA by considering multiple layers of type information:

1. Tracks which types are confined (not cast to incompatible types)
2. For confined types, performs multi-level type matching
3. Handles nested structures and function pointers within structures
4. Applies type constraints to limit the set of possible targets

MLTADF (MLTA with Data Flow)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

This analysis implements data flow tracking on top of MLTA:

1. Performs backward data flow analysis to track function pointer origins
2. Handles common operations like assignments, casts, and structure field access
3. Improves precision by combining type constraints with data flow information

KELP
^^^^

KELP specializes in identifying and analyzing "simple function pointers":

1. A function pointer is considered "simple" if it has no complex uses (like memory objects)
2. For simple function pointers, applies specialized propagation rules
3. Tracks address-taken, copy, phi-node, argument, and call operations

Usage in Code
------------

.. code-block:: cpp

   // Create and run an FPA analysis
   GlobalContext Ctx;
   MLTAPass *pass = new MLTAPass(&Ctx);
   pass->run(Modules);
   
   // Get results for an indirect call
   CallInst *IC = ...;
   FuncSet Targets = Ctx.Callees[IC];

Performance and Precision
-------------------------

The different FPA algorithms offer varying tradeoffs between performance and precision:

- **FLTA**: Fastest but least precise
- **MLTA**: Better precision with moderate performance cost
- **MLTADF**: High precision but more expensive
- **KELP**: Specialized for certain patterns, can be very precise for simple function pointers

The FPA module is particularly effective for analyzing large C/C++ programs where other alias analyses might be too expensive or imprecise. 