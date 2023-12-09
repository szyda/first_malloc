//
// Created by Sylwia Szyda on 16/11/2023.
//

#ifndef EASY_MALLOC_HEAP_C
#define EASY_MALLOC_HEAP_C

#include <stdint.h>
#include <stddef.h>

struct memory_manager_t
{
    void *memory_start; // start address
    size_t memory_size; // all memory avaiable for the allocation
    struct memory_chunk_t *first_memory_chunk; // address of first memory block
};

struct memory_chunk_t
{
    struct memory_chunk_t* prev;
    struct memory_chunk_t* next;
    size_t size;
    int free;
};

#define METADATA sizeof(struct memory_chunk_t)

struct memory_manager_t memory_manager;

void memory_init(void *address, size_t size) {
    if (address != NULL & size > 0) {
        memory_manager.memory_start = address;
        memory_manager.memory_size = size;
        memory_manager.first_memory_chunk = NULL;
    }
}

void *memory_malloc(size_t size) {
    if (size <= 0 || (memory_manager.memory_size - METADATA) < size) {
        return NULL;
    }

    // case when memory is empty
    struct memory_chunk_t *current_block = memory_manager.first_memory_chunk;
    if (current_block == NULL) {
        current_block = (void *)memory_manager.memory_start;
        memory_manager.first_memory_chunk = current_block;

        current_block->prev = NULL;
        current_block->next = NULL;
        current_block->size = size;
        current_block->free = 0;

        return ((void *)current_block + METADATA);
    }

    size_t count_size = 0;
    struct memory_chunk_t *last_memory_block = NULL;

    // case when memory is not empty
    while (current_block) {
        if (current_block->free == 1 && current_block->size >= size + METADATA) {
            current_block->size = size;
            current_block->free = 0;

            return ((void *)current_block + METADATA);
        }
        count_size += current_block->size;
        last_memory_block = current_block;
        current_block = current_block->next;
    }

    // if we didn't find any block
    // we can create block at the end of list if there is enough memory
    if (count_size + METADATA + size <= memory_manager.memory_size) {
        current_block = (void *)((uint8_t *) last_memory_block + last_memory_block->size + METADATA);
        current_block->prev = last_memory_block;
        current_block->next = NULL;
        current_block->size = size;
        current_block->free = 0;

        return (void *)((uint8_t *)current_block + METADATA);
    }

    return NULL;
}

void memory_free(void *address) {

}

#endif //EASY_MALLOC_HEAP_C
