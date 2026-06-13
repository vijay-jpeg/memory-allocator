#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>

typedef struct block {      
    uint32_t size;
    struct block *prev;
    struct block *next;     
    bool free;              
} block_t;

typedef struct {             
    uint32_t total_memory;       
    uint32_t total_blocks;       
    uint32_t used_memory;             
} memory_stats;

static _Thread_local block_t *block_list = NULL; // head of the free list
static _Thread_local memory_stats stats = {0, 0, 0}; // initialize memory statistics

void *my_malloc(uint32_t size) {     // takes the size of memory to be allocated and returns a pointer to the allocated memory
    block_t *current = block_list;
    block_t *smallest_block = NULL;
    block_t *largest_block = NULL;

    while (current && current->next) {
        if (current->size >= size && current->free) {
            if (smallest_block == NULL || current->size < smallest_block->size) {
                smallest_block = current;   // smallest free block that can accommodate the requested size
            }
        }
        if (current->size < size && current->free) {
            if (largest_block == NULL || current->size > largest_block->size) {
                largest_block = current;    // largest block available that is smaller than the requested size
            }
        }
        current = current->next;
    }

    // Check the last block in the list
    if (current){
        if (current->size >= size && current->free) {
            if (smallest_block == NULL || current->size < smallest_block->size) {
                smallest_block = current;
            }
        }
        if (current->size < size && current->free) {
            if (largest_block == NULL || current->size > largest_block->size) {
                largest_block = current;
            }
        }
    }

    if (smallest_block) {
        smallest_block->free = false;
        // move the block to the end of the list
        if (smallest_block->prev) {
            smallest_block->prev->next = smallest_block->next;
        } else {
            block_list = smallest_block->next; // update head if the smallest block is the first block
        }
        if (smallest_block->next) {
            smallest_block->next->prev = smallest_block->prev;
        }
        
        current->next = smallest_block;
        smallest_block->prev = current;

        if (smallest_block->size > size) {
            // return the extra memory to the os
            if (brk(smallest_block + sizeof(block_t) + size) != 0) {
                return NULL; // brk failed
            }
        }

        stats.total_memory += smallest_block->size - size;
        stats.used_memory += size;
        smallest_block->size = size;

        return (void *)(smallest_block + 1);
    } else if (largest_block) {
        largest_block->free = false;
        // move the block to the end of the list
        if (largest_block->prev) {
            largest_block->prev->next = largest_block->next;
        } else {
            block_list = largest_block->next; // update head if the largest block is the first block
        }
        if (largest_block->next) {
            largest_block->next->prev = largest_block->prev;
        }
        current->next = largest_block;
        largest_block->prev = current;

        if (brk(largest_block + sizeof(block_t) + size) != 0) {
            return NULL;
        }

        largest_block->size = size;
        stats.total_memory += size - largest_block->size;
        stats.used_memory += size;
    } else {
        // No suitable block found, request more memory from the system
        block_t *new_block = sbrk(sizeof(block_t) + size);
        if (new_block == (void *)-1) {
            return NULL; // sbrk failed
        }
        new_block->size = size;
        new_block->free = false;
        new_block->next = NULL;
        new_block->prev = NULL;

        if (current) {
            current->next = new_block;
            new_block->prev = current;
        } else {
            block_list = new_block;
        }
        
        stats.total_memory += size;
        stats.used_memory += size;
        stats.total_blocks++;
        return (void *)(new_block + 1);
    }

    return NULL; // should never reach here

}

void my_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    block_t *block = (block_t *)ptr - 1;
    block->free = true;

    stats.used_memory -= block->size;
}

void display_memory() {     // displays the current state of the free list
    block_t *current = block_list;
    printf("Total memory: %u bytes\n", stats.total_memory);
    printf("Used memory: %u bytes\n", stats.used_memory);
    printf("Total blocks: %u\n", stats.total_blocks);

    uint32_t i = 0;
    while (current) {
        printf("Block %d size: %u\n", ++i, current->size);
        current = current->next;
    }
}


int main() {
    void *ptr1 = my_malloc(100);
    void *ptr2 = my_malloc(100);

    assert(stats.total_blocks == 2);
    assert(stats.total_memory == 200);
    assert(stats.used_memory == 200);

    my_free(ptr2);
    assert(stats.used_memory == 100);

    void * ptr3 = my_malloc(150);

    assert(stats.total_blocks == 2);
    assert(stats.total_memory == 250);
    assert(stats.used_memory == 250);
    
    printf("PASSED\n");
}
