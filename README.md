# memory-allocator

This is a simple memory allocator implemented in C. It provides functions for allocating and freeing memory, as well as a function to display the current state of the memory.
## Functions
- `void* malloc(size_t size)`: Allocates a block of memory of the specified size and returns a pointer to it. If there is not enough memory available, it returns NULL.
- `void free(void* ptr)`: Frees a previously allocated block of memory pointed to by `ptr`. If `ptr` is NULL, no operation is performed.
- `void display_memory()`: Displays the current state of the memory, showing allocated and free blocks.