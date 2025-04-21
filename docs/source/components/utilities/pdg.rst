Program Dependence Graph (PDG)
============================

Introduction
-----------

The Program Dependence Graph (PDG) module is a powerful analysis tool that builds inter-procedural program dependence graphs for LLVM IR. The PDG represents both control and data dependencies between program elements, making it valuable for various program analyses, code transformations, and security applications.

This module was originally developed as a key component of the PtrSplit and Program-mandering projects. It builds a modular inter-procedural program dependence graph that is field-sensitive, context-insensitive, and flow-insensitive.

Overview
--------

The PDG module contains several components that work together to build and represent program dependencies:

- **Graph Infrastructure**: Core classes for representing graphs, nodes, and edges
- **Dependency Analysis**: Components for analyzing control and data dependencies
- **Function/Call Handling**: Wrappers and utilities for handling functions and function calls
- **Visualization**: Tools for visualizing the dependency graphs

Key Components
-------------

Graph Structure
~~~~~~~~~~~~~~

- **ProgramGraph**: The main graph class that represents the entire program's dependency structure
- **GenericGraph**: Base class for graph implementations
- **Node**: Represents a program element (instruction, function, variable, etc.)
- **Edge**: Represents a dependency relationship between nodes
- **Tree**: Hierarchical structure for organizing related nodes

Dependency Analysis
~~~~~~~~~~~~~~~~~

- **ProgramDependencyGraph**: The main pass for building the complete PDG
- **DataDependencyGraph**: Analyzes and builds data dependencies
- **ControlDependencyGraph**: Analyzes and builds control dependencies
- **PDGCallGraph**: Specialized call graph with PDG-specific information

Wrappers and Utilities
~~~~~~~~~~~~~~~~~~~~~

- **FunctionWrapper**: Wrapper for function-related PDG components
- **CallWrapper**: Wrapper for call instruction-related PDG components
- **PDGUtils**: Utility functions for PDG operations
- **DebugInfoUtils**: Utilities for working with debug information

Node and Edge Types
~~~~~~~~~~~~~~~~~~

The PDG represents various types of program elements as nodes:

- Instructions (calls, returns, branches, etc.)
- Functions and function entries
- Parameters (formal and actual)
- Variables (global, static, etc.)
- Classes and annotations

The edges in the PDG represent different types of dependencies:

- Control dependencies (branch, call, etc.)
- Data dependencies (def-use, RAW, etc.)
- Parameter edges (in, out, field)
- Other dependencies (global, value, class-method, etc.)

Using the PDG Module
-------------------

The PDG module can be used as an LLVM pass or as a required analysis in your own passes.

Available Passes
~~~~~~~~~~~~~~

- ``-pdg``: Generate the program dependence graph (inter-procedural)
- ``-cdg``: Generate the control dependence graph (intra-procedural)
- ``-ddg``: Generate the data dependence graph (intra-procedural)
- ``-dot-*``: For visualization (using dot/Graphviz)

As a Required Analysis
~~~~~~~~~~~~~~~~~~~~

To use PDG as a required analysis in your pass:

.. code-block:: cpp

    AU.addRequired<ProgramDependencyGraph>();

Key APIs
~~~~~~~

**Query node reachability:**

.. code-block:: cpp

    ProgramGraph *g = getAnalysis<ProgramDependencyGraph>()->getPDG();
    Value* src;
    Value* dst;
    pdg::Node* src_node = g->getNode(*src);
    pdg::Node* dst_node = g->getNode(*dst);
    
    if (g->canReach(src_node, dst_node)) 
    {
      // Do something...
    }

**Traverse with path constraints:**

.. code-block:: cpp

    ProgramGraph *g = getAnalysis<ProgramDependencyGraph>()->getPDG();
    Value* src;
    Value* dst;
    pdg::Node* src_node = g->getNode(*src);
    pdg::Node* dst_node = g->getNode(*dst);
    
    std::set<pdg::EdgeType> exclude_edges;
    // Add edge types to exclude
    
    if (g->canReach(src_node, dst_node, exclude_edges)) 
    {
      // Do something...
    }

Limitations
----------

- The current implementation only supports building PDGs for C programs
- For large programs, generating visualizable PDG files can be challenging due to the size
- The implementation is based on LLVM 14.0.0

References
---------

For more information, see the PtrSplit paper:

    Shen Liu, Gang Tan, Trent Jaeger. "PtrSplit: Supporting General Pointers in Automatic Program Partitioning." 
    In 24th ACM Conference on Computer and Communications Security (CCS), pages 2359-2371, 2017.

The PDG module is an essential utility for various program analyses and transformations, providing a detailed view of program dependencies that can be leveraged for many applications, including program slicing, security analysis, and code optimization.
