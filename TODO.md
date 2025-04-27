# Development

For Summer Research, Final Year Project Topics, etc.

## Testing Improvements

- Different OS, LLVM, Z3, Boost, etc.
- Use lib/Alias/Dynamic to test pointer analyses.

## Alias Analysis

### Interfaces

* Universal Interface for the analyses in lib/Alias: points-to, alias pair, alias set, pointed-by, callgraph memory dependence, etc.
* Use lib/Annotation to enable the loading of aliasing specification files for the pointer analyses (And perhpas use dynamic analysis/ML to extract specifications)

### Integration

* Integrate SVF and POCR as a library


## Intermediate Representations (IR) 

### VFG (Value Flow Graph)

The DyckAA module has a DyckVFG, which is used by the null pointer analysis (lib/NullPointer).
Maybe we can design a VFG independent from DyckAA (e.g., it can use the results of other analyses)

### PDG (Program Dependence Graph)

* Use the pointer analysis interfaces (currently, it relies on the memory dependence analysis inside LLVM)

## Slicing

Use VFG or PDG to answer slicing queries.

* Forward slicing
* Backward slicing
* Program chopping
* Thin slicing

(Maybe refer to the implementation in DG)

## Numerical Analysis

* Integrate abstract interpretation frameworks (e.g., IKOS, CLAM/Crab)

## Data Flow Analysis

* Revise the current data flow analysis module

## End-to-End

- Null pointer analysis (lib/NullPointer)
- Buffer overflow detection?
- Memory leak detection?
- Race condition analysis?


