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
pthread_mutex_t brk_lock = PTHREAD_MUTEX_INITIALIZER;

void *my_malloc(uint32_t size) {     // takes the size of memory to be allocated and returns a pointer to the allocated memory
    block_t *current = block_list;
    block_t *smallest_block = NULL;

    while (current && current->next) {
        if (current->size >= size && current->free) {
            if (smallest_block == NULL || current->size <= smallest_block->size) {
                smallest_block = current;   // smallest free block that can accommodate the requested size
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
    }

    if (smallest_block) {
        smallest_block->free = false;

        // if the block is the newest allocated one
        if (smallest_block == current) {
            if (smallest_block->size - size >= sizeof(block_t) + 16) {     // ensure it's worth trimming
                // move the break downward to return memory to the OS
                pthread_mutex_lock(&brk_lock);
                brk((char *)smallest_block + sizeof(block_t) + size);
                stats.total_memory -= smallest_block->size - size;
                pthread_mutex_unlock(&brk_lock);
            }
        }

        else if (smallest_block->size >= size + sizeof(block_t) + 16) {   // ensure there is enough extra space for splitting
            block_t *split_block = (block_t *) ((char *)(smallest_block + 1) + size);
            split_block->size = smallest_block->size - size - sizeof(block_t);
            split_block->free = true;
            split_block->next = smallest_block->next;
            if (smallest_block->next) {
                smallest_block->next->prev = split_block;
            }
            smallest_block->next = split_block;
        }
        

        smallest_block->size = size;
        stats.used_memory += size;

        return (void *)(smallest_block + 1);
    } else if (current && current->free) {       // if no block has enough space, extend the last one if it is free
        current->free = false;

        pthread_mutex_lock(&brk_lock);
        brk((char *)current + sizeof(block_t) + size);
        pthread_mutex_unlock(&brk_lock);

        current->size = size;
        stats.total_memory += size - current->size;
        stats.used_memory += size;

        return (void *)(current + 1);
    } else {
        // No suitable block found, request more memory from the system
        pthread_mutex_lock(&brk_lock);
        block_t *new_block = sbrk(sizeof(block_t) + size);
        pthread_mutex_unlock(&brk_lock);

        if (new_block == (void *)-1) {
            return NULL; // sbrk failed
        }
        new_block->size = size;
        new_block->free = false;
        new_block->next = NULL;
        new_block->prev = NULL;

        if (current) {      // if block list is not empty
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
    assert(ptr1 != NULL);
    
    // verify stats tracking for a single block
    assert(stats.total_blocks == 1);
    assert(stats.used_memory == 100);

    void *ptr2 = my_malloc(200);
    assert(ptr2 != NULL);
    assert(ptr1 != ptr2); // pointers must be distinct

    // ensure payloads do not overlap
    if (ptr1 < ptr2) {
        assert((char *)ptr1 + 100 <= (char *)ptr2);
    } else {
        assert((char *)ptr2 + 200 <= (char *)ptr1);
    }

    assert(stats.total_blocks == 2);
    assert(stats.used_memory == 300);

    unsigned char *c1 = (unsigned char *)ptr1;
    unsigned char *c2 = (unsigned char *)ptr2;
    
    for (int i = 0; i < 100; i++) c1[i] = 0xAA;
    for (int i = 0; i < 200; i++) c2[i] = 0xBB;

    // verify data remained uncorrupted after filling adjacent blocks
    for (int i = 0; i < 100; i++) assert(c1[i] == 0xAA);
    for (int i = 0; i < 200; i++) assert(c2[i] == 0xBB);

    my_free(ptr1);
    assert(stats.used_memory == 200); // only ptr2 (200 bytes) should be marked active

    // test double-free safety or NULL handling if your code allows it
    my_free(NULL); 
    assert(stats.used_memory == 200);

    // requesting exactly 100 bytes again should recycle the block pointed to by ptr1
    void *ptr3 = my_malloc(100);
    assert(ptr3 == ptr1); 
    assert(stats.used_memory == 300);
    // total blocks should not increase if an entire existing block was recycled
    assert(stats.total_blocks == 2); 

    // requesting a size larger than any free block should trigger a new system allocation
    void *ptr4 = my_malloc(500);
    assert(ptr4 != NULL);
    assert(ptr4 != ptr1 && ptr4 != ptr2);
    assert(stats.total_blocks == 3);
    assert(stats.used_memory == 800);

    pthread_mutex_destroy(&brk_lock);

    printf("All assertions passed\n");
    return 0;
}
