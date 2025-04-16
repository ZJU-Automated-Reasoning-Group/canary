Lotus Tools
===========

This section provides information about the various tools available in the Lotus framework and how to use them effectively.

.. toctree::
   :maxdepth: 1

   kint
   csr
   canary
   seadsa-tool
   owl

Available Tools
---------------

- :doc:`Kint <kint>`: Integer bug detector
- :doc:`CSR <csr>`: Context-sensitive reachability indexing
- :doc:`Canary <canary>`: Null pointer analysis
- :doc:`Sea-DSA <seadsa-tool>`: Data structure analysis
- :doc:`OWL <owl>`: SMT solver

Tool Usage Examples
-------------------

Kint Bug Detector
~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Enable all checkers
   ./kint input.ll --check-all=true

   # Enable specific checkers
   ./kint input.ll --check-int-overflow=true --check-div-by-zero=true

Context-Sensitive Reachability Indexing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Run CSR with Grail indexing
   ./csr -n 1000 -m grail ./dataset/example.txt

For detailed usage instructions for each tool, refer to the specific tool documentation pages listed above. 