Context-Sensitive Reachability Indexing
=======================================

Context-Sensitive Reachability (CSR) Indexing is a technique for efficiently computing reachability queries in context-sensitive program analysis.

Overview
--------

This tool tests how existing indexing schemes (pathtree and grail) speed up context-sensitive reachability. For more information on indexing context-sensitive reachability, refer to:

.. code-block:: text

   OOPSLA'22: Indexing the Extended Dyck-CFL Reachability for Context-Sensitive Program Analysis
   Qingkai Shi, Yongchao Wang, Peisen Yao, Charles Zhang
   The ACM SIGPLAN Conference on Objected Oriented Programming, Systems, Languages, and Applications

Building CSR
------------

The CSR tool is built as part of the Lotus framework. Follow the general :doc:`installation instructions </getting_started/installation>` to build the entire framework.

Dataset Format
--------------

The CSR tool works with program-valid graphs. Each graph file is in the following format:

.. code-block:: text

   graph_for_greach
   6
   0: 1 2.1 #0
   1: 2.2 #0
   2: 3 #1
   3: 4.-1 5.-2 #1
   4: #0
   5: #0

The first line of the graph file is just a string ``graph_for_greach`` to indicate the format of the file.

The second line indicates the number of vertices in the graph.

The following lines represents the graph as an adjacency list where the vertices are consecutive natural numbers,
which are in the format below: 

.. code-block:: text

   source: (target[.[-]call_id] )*#function_id

For example:
``0: 1 2.1 #0`` means the vertex ``0`` is in the function ``0`` and there are two edges:

* the edge from ``0`` to ``1``, which is an intra-procedural edge
* the edge from ``0`` to ``2``, which is a call edge at the call site ``1``.

For example:
``3: 4.-1 5.-2 #1`` means the vertex ``3`` is in the function ``1`` and there are two edges:

* the edge from ``3`` to ``4``, which is a return edge at the call site ``1``.
* the edge from ``3`` to ``5``, which is a return edge at the call site ``2``.

Running the CSR Tool
--------------------

.. code-block:: text

   $ ./csr -h

   Usage:
           csr [-h] [-t] [-m pathtree_or_grail] [-d grail_dim] [-n num_query] [-q query_file] [-g query_file] graph_file
   Description:
           -h      Print the help message.
           -n      # reachable queries and # unreachable queries to be generated, 100 for each by default.
           -g      Save the randomly generated queries into file.
           -q      Read the randomly generated queries from file.
           -t      Evaluate transitive closure.
           -r      Evaluate rep's tabulation algorithm.
           -m      Evaluate what indexing approach, pathtree, grail, or pathtree+grail.
           -d      Set the dim of Grail, 2 by default.

Sample Usage
~~~~~~~~~~~~

.. code-block:: bash

   $ ./csr -n 1000 -m grail ./dataset/mcf.txt

   .....
   .....
   .....
   --------- GRAIL Queries Test ------------
   Grail for 1000 reachable queries: 4 ms. Success rate: 100 %.
   Grail for 1000 unreachable queries: 0 ms. Success rate: 100 %.
   --------- Indexing Construction Summary ---------
   #Vertices: 22194
   #Edges: 29372
   #Summary Edges: 2452
   Summary Edge     time: 10.58 ms. 
   Summary Edge     size: 0.02 mb.
   GRAIL    indices time: 36.76 ms. 
   GRAIL    indices size: 1.19 mb. 

Acknowledgement
---------------

This tool includes source code contributed by the authors of `PathTree <http://www.cs.kent.edu/~nruan/soft.html>`_ and `Grail <https://github.com/zakimjz/grail>`_. 