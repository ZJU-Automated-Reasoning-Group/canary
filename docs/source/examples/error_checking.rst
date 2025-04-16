Error Checking Bug Detection
============================

This example demonstrates how to use Lotus's ESSS tool to detect error checking bugs in C code.

Example Code
------------

Consider this code with potential error checking issues:

.. code-block:: c

    #include <stdio.h>
    #include <stdlib.h>
    #include <errno.h>
    #include <string.h>
    #include <fcntl.h>
    #include <unistd.h>

    int process_file(const char *filename) {
        int fd;
        char buffer[1024];
        ssize_t bytes_read;
        
        // Open the file
        fd = open(filename, O_RDONLY);
        
        // BUG: Missing check for error return value
        
        // Read from file
        bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        
        // BUG: Missing check for error return value from read
        
        // Null-terminate the buffer
        buffer[bytes_read] = '\0';
        printf("Read: %s\n", buffer);
        
        // Close file
        close(fd);
        
        return 0;
    }

    int main(int argc, char *argv[]) {
        if (argc < 2) {
            printf("Usage: %s <filename>\n", argv[0]);
            return 1;
        }
        
        return process_file(argv[1]);
    }

Analysis Steps
--------------

1. Compile the code to LLVM bitcode:

.. code-block:: bash

    clang -emit-llvm -c -g error_example.c -o error_example.bc

2. Run ESSS on the bitcode:

.. code-block:: bash

    esss error_example.bc

3. ESSS will report error checking issues:

.. code-block:: text

    Warning: Return value from open() not checked at error_example.c:14
    Warning: Return value from read() not checked at error_example.c:18
    Warning: Potential resource leak on error path at error_example.c:14-25

Fixing the Issues
-----------------

Here's an improved version with proper error checking:

.. code-block:: c

    #include <stdio.h>
    #include <stdlib.h>
    #include <errno.h>
    #include <string.h>
    #include <fcntl.h>
    #include <unistd.h>

    int process_file(const char *filename) {
        int fd;
        char buffer[1024];
        ssize_t bytes_read;
        
        // Open the file with error checking
        fd = open(filename, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Error opening file %s: %s\n", 
                    filename, strerror(errno));
            return -1;
        }
        
        // Read from file with error checking
        bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read == -1) {
            fprintf(stderr, "Error reading from file: %s\n", 
                    strerror(errno));
            close(fd);  // Prevent resource leak
            return -1;
        }
        
        // Null-terminate the buffer
        buffer[bytes_read] = '\0';
        printf("Read: %s\n", buffer);
        
        // Close file
        if (close(fd) == -1) {
            fprintf(stderr, "Error closing file: %s\n", 
                    strerror(errno));
            return -1;
        }
        
        return 0;
    }

    int main(int argc, char *argv[]) {
        if (argc < 2) {
            printf("Usage: %s <filename>\n", argv[0]);
            return 1;
        }
        
        return process_file(argv[1]);
    } 