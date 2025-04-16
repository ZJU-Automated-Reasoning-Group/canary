Null Pointer Detection
======================

This example demonstrates how to use Lotus to detect potential null pointer dereferences in C code.

Example Code
------------

Consider the following C code with potential null pointer issues:

.. code-block:: c

    #include <stdlib.h>
    #include <stdio.h>

    void process(char *data) {
        // Potential null dereference
        printf("First character: %c\n", *data);
    }

    int main() {
        char *ptr = NULL;
        
        // Some condition that might set ptr
        if (rand() % 2) {
            ptr = malloc(10);
            if (ptr) {
                ptr[0] = 'A';
            }
        }
        
        // Potential null pointer passed to function
        process(ptr);
        
        if (ptr) {
            free(ptr);
        }
        
        return 0;
    }

Analysis Steps
--------------

1. Compile the code to LLVM bitcode:

.. code-block:: bash

    clang -emit-llvm -c -g null_example.c -o null_example.bc

2. Run Canary on the bitcode:

.. code-block:: bash

    canary null_example.bc

3. Canary will analyze the code and produce output similar to:

.. code-block:: text

    Warning: Potential null pointer dereference at null_example.c:6
      Call to process() at null_example.c:20 may pass NULL value

Fixing the Issues
-----------------

To fix these issues, you should add proper null checks:

.. code-block:: c

    void process(char *data) {
        if (data != NULL) {
            printf("First character: %c\n", *data);
        } else {
            printf("Data is NULL\n");
        }
    }

    int main() {
        char *ptr = NULL;
        
        if (rand() % 2) {
            ptr = malloc(10);
            if (ptr) {
                ptr[0] = 'A';
            }
        }
        
        // Check before passing to function
        if (ptr != NULL) {
            process(ptr);
        } else {
            printf("Cannot process NULL data\n");
        }
        
        if (ptr) {
            free(ptr);
        }
        
        return 0;
    } 