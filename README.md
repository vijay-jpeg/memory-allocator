# memory-allocator

This is a simple memory allocator implemented in C. It provides functions for allocating and freeing memory, as well as a function to display the current state of the memory.
## Functions
- `void* malloc(size_t size)`: Allocates a block of memory of the specified size and returns a pointer to it. If the allocation fails, it returns NULL.
- On allocation, the allocator searches for a free block of memory that is large enough to accommodate the requested size. If such a block is found, it is marked as allocated and a pointer to the beginning of the block's free space is returned. If no suitable block is found, requests only the necessary amount of memory from the operating system; or return NULL if it cannot fulfill the request.
- If the suitable block is much larger than the requested size, the allocator splits the block into two parts: one part is allocated to the request, and the other part remains free for future allocations.
- If no suitable block is found, the allocator first checks if the block added most recently is free. If so, it only requests the difference between the requested size and the size of the most recently added block from the operating system.
- `void free(void* ptr)`: Frees a previously allocated block of memory pointed to by `ptr` by marking the it as free so it can be reused for future allocations. If `ptr` is NULL, no operation is performed.
- `void display_memory()`: Displays the current state of the memory, showing allocated and free blocks.