/*
 * Test case 1: Function with different calling contexts
 * 
 * This test demonstrates how context-sensitive analysis can distinguish
 * between different calling contexts of the same function.
 * 
 * The 'process_ptr' function is called from two different contexts:
 * 1. From 'safe_context' where the pointer is guaranteed to be non-null
 * 2. From 'unsafe_context' where the pointer may be null
 * 
 * A context-sensitive analysis should be able to determine that the pointer
 * in 'process_ptr' is definitely non-null when called from 'safe_context',
 * but may be null when called from 'unsafe_context'.
 */

#include <stdio.h>
#include <stdlib.h>

// Process a pointer - the nullness depends on the calling context
void process_ptr(char *ptr) {
    // A context-sensitive analysis should know that ptr is non-null when called from safe_context
    // but may be null when called from unsafe_context
    if (ptr != NULL) {
        printf("Pointer is not NULL: %c\n", *ptr);
    } else {
        printf("Pointer is NULL\n");
    }
}

// This context guarantees a non-null pointer
void safe_context() {
    char *ptr = malloc(10);
    if (ptr == NULL) {
        return; // Early return if allocation fails
    }
    ptr[0] = 'A';
    
    // Here, ptr is guaranteed to be non-null
    process_ptr(ptr);
    
    free(ptr);
}

// This context may pass a null pointer
void unsafe_context(int condition) {
    char *ptr = NULL;
    
    if (condition > 0) {
        ptr = malloc(10);
        if (ptr != NULL) {
            ptr[0] = 'B';
        }
    }
    
    // Here, ptr may be null depending on the condition
    process_ptr(ptr);
    
    if (ptr != NULL) {
        free(ptr);
    }
}

int main() {
    safe_context();
    unsafe_context(0);  // Will pass NULL
    unsafe_context(1);  // May pass non-NULL
    return 0;
} 