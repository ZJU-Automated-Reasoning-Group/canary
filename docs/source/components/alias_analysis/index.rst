Alias Analysis
==============

This section covers the alias analysis components available in the Lotus framework.

.. toctree::
   :maxdepth: 1

   dyckaa
   cflaa
   sea_dsa
   andersen
   fpa

Lotus provides several alias analysis implementations with different precision and performance characteristics:

- :doc:`DyckAA <dyckaa>`: A unification-based, exhaustive alias analysis
- :doc:`CFLAA <cflaa>`: Context-Free Language Reachability-based Alias Analysis
- :doc:`Sea-DSA <sea_dsa>`: A context-sensitive, pointer-sensitivity alias analysis based on the Data Structure Analysis (DSA, PLDI'07)
- :doc:`Andersen <andersen>`: Context-insensitive points-to analysis implementation
- :doc:`FPA <fpa>`: Specialized function pointer analysis with multiple approaches (FLTA, MLTA, KELP)

These alias analyses can be used as standalone tools or as part of larger program analysis workflows. 