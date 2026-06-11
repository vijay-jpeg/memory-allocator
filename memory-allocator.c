#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

typedef struct block {      
    uint32_t size;          
    struct block *next;     
    bool free;              
} block_t;

typedef struct {             
    uint32_t total_memory;       
    uint32_t total_blocks;       
    uint32_t used_memory;             
} memory_stats;

void *malloc(size_t size) {     // takes the size of memory to be allocated and returns a pointer to the allocated memory
    static block_t *block_list = NULL;
    block_t *current = block_list;
    block_t *smallest_block = NULL;
    while (current) {
        if (current->free && current->size >= size) {
            if (smallest_block == NULL || current->size < smallest_block->size) {
                smallest_block = current;
            }
        }
        current = current->next;
    }

    if (smallest_block) {
        smallest_block->free = false;
        return (void *)(smallest_block + 1);
    }

    block_t *new_block = sbrk(sizeof(block_t) + size);
    if (new_block == (void *)-1) {
        return NULL; // sbrk failed
    }
    new_block->size = size;
    new_block->free = false;
    new_block->next = block_list;
    return (void *)(new_block + 1);
}

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    block_t *block = (block_t *)ptr - 1;
    block->free = true;
}

void display_memory() {     // displays the current state of the free list
    static block_t *free_list = NULL;
    block_t *current = free_list;

    static memory_stats stats = {0, 0, 0};
    printf("Total memory: %zu bytes\n", stats.total_memory);
    printf("Used memory: %zu bytes\n", stats.used_memory);
    printf("Total blocks: %zu\n", stats.total_blocks);

    while (current) {
        printf("Block size: %zu\n", current->size);
        current = current->next;
    }
}

