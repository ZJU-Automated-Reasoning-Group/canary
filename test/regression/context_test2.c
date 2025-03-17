/*
 * Test case 2: Interprocedural null check propagation
 * 
 * This test demonstrates how context-sensitive analysis can track null checks
 * across function boundaries and propagate this information back to callers.
 * 
 * The 'validate_ptr' function checks if a pointer is null and returns a boolean.
 * The caller then uses this information to make decisions.
 * 
 * A context-sensitive analysis should be able to determine that after a successful
 * validation, the pointer is definitely non-null in the specific calling context.
 */

#include <stdio.h>
#include <stdlib.h>

// Validate that a pointer is non-null
int validate_ptr(void *ptr) {
    return (ptr != NULL);
}

// Use the validation result to make decisions
void use_ptr_safely(void *ptr) {
    if (validate_ptr(ptr)) {
        // A context-sensitive analysis should know that ptr is non-null here
        // because we've checked it through validate_ptr
        printf("Pointer value: %p\n", ptr);
        
        // This dereference is safe because we've validated the pointer
        *((char*)ptr) = 'X';
    } else {
        printf("Invalid pointer, not using it\n");
    }
}

// Another function that uses validation in a different way
void conditional_use(void *ptr, int condition) {
    // First use without validation
    if (condition > 0 && ptr != NULL) {
        printf("Condition met, using pointer: %p\n", ptr);
    }
    
    // Now validate and use
    if (validate_ptr(ptr)) {
        // A context-sensitive analysis should know ptr is non-null here
        *((char*)ptr) = 'Y';
    }
}

int main() {
    // Test with various pointers
    void *ptr1 = NULL;
    void *ptr2 = malloc(10);
    
    use_ptr_safely(ptr1);  // Should not dereference
    use_ptr_safely(ptr2);  // Should safely dereference
    
    conditional_use(ptr1, 1);  // Should not dereference
    conditional_use(ptr2, 0);  // Should safely dereference in second check
    
    if (ptr2) free(ptr2);
    return 0;
} 