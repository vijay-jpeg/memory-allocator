#include <unistd.h>

typedef struct block {      // to save each block of memory allocated
    size_t size;            // size of the block
    struct block *next;     // pointer to the next block in the free list
} block_t;

void *malloc(size_t size) {     // takes the size of memory to be allocated and returns a pointer to the allocated memory
    static block_t *free_list = NULL;       // pointer to the head of the free list
    block_t *current = free_list, *prev = NULL;  // pointers to traverse the free list

    while (current) {
        if (current->size >= size) {    // if the current block is large enough to accommodate the requested size
            if (prev) {     // if the block is not the head of the free list
            // remove the block from the free list, by updating the next pointer of the previous block to point to the next block
                prev->next = current->next;     
            } else {
            // if the block is the head of the free list, update the head to point to the next block
                free_list = current->next;
            }
            // return a pointer to the memory immediately following the block header, which is the usable memory for the caller
            return (void *)(current + 1);
        }
        prev = current;
        current = current->next;
    }

    // if no suitable block is found in the free list, request more memory from the operating system using sbrk
    block_t *new_block = sbrk(sizeof(block_t) + size);  
    if (new_block == (void *)-1) {  // failed to allocate memory
        return NULL; 
    }
    new_block->size = size;     
    return (void *)(new_block + 1);
}

void free(void *ptr) {      // takes a pointer to the memory to be freed and adds it back to the free list
    if (!ptr) return;

    block_t *block = (block_t *)ptr - 1;  
    block->next = NULL;

    static block_t *free_list = NULL;
    block->next = free_list;        // append the freed block to the front of the free list
    free_list = block;
}

void display_memory() {     // displays the current state of the free list
    static block_t *free_list = NULL;
    block_t *current = free_list;

    while (current) {
        printf("Block size: %zu\n", current->size);
        current = current->next;
    }
}

