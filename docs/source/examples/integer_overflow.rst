Integer Overflow Detection
==========================

This example shows how to use Lotus's Kint tool to detect integer overflow bugs in C code.

Example Code
------------

Consider this example with potential integer overflow issues:

.. code-block:: c

    #include <stdlib.h>
    #include <stdio.h>

    void *allocate_buffer(size_t n_elements, size_t element_size) {
        // Potential integer overflow in multiplication
        size_t size = n_elements * element_size;
        return malloc(size);
    }

    int main(int argc, char *argv[]) {
        if (argc < 3) {
            printf("Usage: %s <count> <size>\n", argv[0]);
            return 1;
        }
        
        // Convert command line arguments to integers
        size_t count = (size_t)atoi(argv[1]);
        size_t size = (size_t)atoi(argv[2]);
        
        // Potential overflow in allocate_buffer
        void *buffer = allocate_buffer(count, size);
        
        if (!buffer) {
            printf("Allocation failed\n");
            return 1;
        }
        
        // Use buffer...
        free(buffer);
        return 0;
    }

Analysis Steps
--------------

1. Compile the code to LLVM bitcode:

.. code-block:: bash

    clang -emit-llvm -c -g overflow_example.c -o overflow_example.bc

2. Run Kint on the bitcode:

.. code-block:: bash

    kint overflow_example.bc

3. Kint will detect and report potential integer overflow issues:

.. code-block:: text

    Warning: Potential integer overflow at overflow_example.c:6
      size_t size = n_elements * element_size;
      Overflow possible when n_elements and element_size are large

Fixing the Issues
-----------------

To fix the integer overflow issue, you can use safe multiplication:

.. code-block:: c

    #include <stdlib.h>
    #include <stdio.h>
    #include <stdint.h>

    void *allocate_buffer(size_t n_elements, size_t element_size) {
        // Check for multiplication overflow
        if (n_elements > 0 && element_size > SIZE_MAX / n_elements) {
            return NULL; // Overflow would occur
        }
        
        size_t size = n_elements * element_size;
        return malloc(size);
    }

    int main(int argc, char *argv[]) {
        if (argc < 3) {
            printf("Usage: %s <count> <size>\n", argv[0]);
            return 1;
        }
        
        // Convert command line arguments to integers with bounds checking
        long count_long = atol(argv[1]);
        long size_long = atol(argv[2]);
        
        if (count_long <= 0 || size_long <= 0 || 
            count_long > (long)SIZE_MAX || size_long > (long)SIZE_MAX) {
            printf("Invalid input values\n");
            return 1;
        }
        
        size_t count = (size_t)count_long;
        size_t size = (size_t)size_long;
        
        void *buffer = allocate_buffer(count, size);
        
        if (!buffer) {
            printf("Allocation failed\n");
            return 1;
        }
        
        // Use buffer...
        free(buffer);
        return 0;
    } 