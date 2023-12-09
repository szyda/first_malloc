#include <stdint.h>
#include <stddef.h>
#include "heap.h"

#define METADATA sizeof(struct memory_chunk_t)

void memory_init(void *address, size_t size) {
    if (address != NULL & size > 0) {
        memory_manager.memory_start = address;
        memory_manager.memory_size = size;
        memory_manager.first_memory_chunk = NULL;
    }
}

void *memory_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // case when memory is empty
    struct memory_chunk_t *current_block = memory_manager.first_memory_chunk;
    if (current_block == NULL) {
        if (size + METADATA > memory_manager.memory_size) {
            return NULL;
        }
        current_block = (void *)memory_manager.memory_start;
        memory_manager.first_memory_chunk = current_block;

        current_block->prev = NULL;
        current_block->next = NULL;
        current_block->size = size;
        current_block->free = 0;

        return (void *) ((uint8_t *)current_block + METADATA);
    }

    size_t count_size = 0;
    struct memory_chunk_t *last_memory_block = NULL;

    // case when memory is not empty
    while (current_block) {
        if (current_block->free == 1 && current_block->size >= size + METADATA) {
            current_block->size = size;
            current_block->free = 0;

            return (void *) ((uint8_t *)current_block + METADATA);
        }
        count_size += current_block->size + METADATA;
        last_memory_block = current_block;
        current_block = current_block->next;
    }

    // if we didn't find any block
    // we can create block at the end of list if there is enough memory
    if (count_size + METADATA + size <= memory_manager.memory_size) {
        current_block = (void *)((uint8_t *) last_memory_block + last_memory_block->size + METADATA);
        current_block->prev = last_memory_block;
        last_memory_block->next = current_block;

        current_block->next = NULL;
        current_block->size = size;
        current_block->free = 0;

        return (void *)((uint8_t *)current_block + METADATA);
    }

    return NULL;
}

void memory_free(void *address) {
    if (address != NULL) {
        struct memory_chunk_t *current_metadata = (void *)((uint8_t *)address - METADATA);

        if (current_metadata->free != 0 && current_metadata->free != 1) return;
        if (current_metadata->size == 0) return;

        // check address
        if (validate_metadata(current_metadata)) {
            return;
        }

        // update size if there is a next block
        if (current_metadata->next != NULL) {
            current_metadata->size = ((uint8_t *)current_metadata->next) - ((uint8_t *)current_metadata + METADATA);
        }

        current_metadata->free = 1;

        // merge with previous block if free
        while (current_metadata->prev && current_metadata->prev->free == 1) {
            current_metadata = current_metadata->prev;
            current_metadata->size += current_metadata->next->size + METADATA;
            current_metadata->next = current_metadata->next->next;
            if (current_metadata->next) {
                current_metadata->next->prev = current_metadata;
            }
        }

        // merge with next block if free
        while (current_metadata->next && current_metadata->next->free == 1) {
            current_metadata->size += current_metadata->next->size + METADATA;
            current_metadata->next = current_metadata->next->next;
            if (current_metadata->next) {
                current_metadata->next->prev = current_metadata;
            }
        }

        // update pointers
        if (current_metadata->next == NULL && current_metadata->prev != NULL) {
            current_metadata->prev->next = NULL;
        }

        if (memory_manager.first_memory_chunk == current_metadata && current_metadata->next == NULL) {
            memory_manager.first_memory_chunk = NULL;
        }
    }
}

int validate_address(struct memory_chunk_t *metadata) {
    struct memory_chunk_t *current_metadata = memory_manager.first_memory_chunk;
    while (current_metadata) {
        if (current_metadata == metadata) {
            return 0;
        }
        current_metadata = current_metadata->next;
    }

    return 1;
}




