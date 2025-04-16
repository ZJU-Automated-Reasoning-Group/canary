Testing
=======

(This is a placeholder for the testing section of the dev guide.)

Setting Up the Test Environment
-------------------------------

Before running tests, make sure you have:

1. Built Lotus with the ``LOTUS_ENABLE_TESTS`` option
2. Installed any test dependencies

Running Tests
-------------

Basic Test Suite
~~~~~~~~~~~~~~~~

To run the full test suite:

.. code-block:: bash

    cd build
    make test

Running Specific Tests
~~~~~~~~~~~~~~~~~~~~~~

To run a specific test or test category:

.. code-block:: bash

    cd build
    ctest -R <test-pattern>

For example, to run only the Sea-DSA tests:

.. code-block:: bash

    ctest -R sea-dsa

Test Structure
--------------

Unit Tests
~~~~~~~~~~

Unit tests are organized by component and test specific functionality in isolation:

* ``test/alias_analysis/`` - Tests for alias analysis components
* ``test/constraint_solving/`` - Tests for constraint solving components
* ``test/bug_finding/`` - Tests for bug finding tools

Integration Tests
~~~~~~~~~~~~~~~~~

Integration tests verify the interaction between multiple components:

* ``test/integration/`` - Tests for component interactions
* ``test/tools/`` - End-to-end tests for command-line tools

Adding New Tests
----------------

1. Create a new test file in the appropriate directory
2. Add the test to the corresponding CMakeLists.txt file
3. Document what the test is verifying
4. Add any necessary test data files

Continuous Integration
----------------------

The project uses GitHub Actions for continuous integration:

* All pull requests are automatically tested
* The main branch is tested on each commit
* Tests run on multiple platforms (Linux, macOS) 