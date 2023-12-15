#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "heap.h"

#define METADATA sizeof(struct memory_chunk_t)
#define FENCE 'x'
#define FENCE_SIZE 16

void memory_init(void *address, size_t size) {
    if (address != NULL && size > 0) {
        memory_manager.memory_start = address;
        memory_manager.memory_size = size;
        memory_manager.first_memory_chunk = NULL;
    }
}

void *memory_malloc(size_t size) {
    if (size == 0) return NULL;
    if (heap_validate()) return NULL;

    // case when memory is empty
    struct memory_chunk_t *current_block = memory_manager.first_memory_chunk;
    if (current_block == NULL) {
        if (size + METADATA > memory_manager.memory_size) return NULL;

        current_block = (struct memory_chunk_t*) memory_manager.memory_start;
        memory_manager.first_memory_chunk = current_block;

        current_block->prev = NULL;
        current_block->next = NULL;
        current_block->size = size;
        current_block->free = 0;

        memset(((uint8_t *)current_block + METADATA), FENCE, FENCE_SIZE);
        memset(((uint8_t *)current_block + METADATA + size + FENCE_SIZE), FENCE, FENCE_SIZE);

        return (void *) ((uint8_t *)current_block + METADATA + FENCE_SIZE);
    }

    size_t count_size = 0;
    struct memory_chunk_t *last_memory_block = NULL;
    current_block = memory_manager.memory_start;

    // case when memory is not empty
    while (current_block) {
        if (current_block->free == 1 && current_block->size >= size + METADATA + 2 * FENCE_SIZE) {
            current_block->size = size;
            current_block->free = 0;

            memset((uint8_t *)current_block + METADATA, FENCE, FENCE_SIZE);
            memset((uint8_t *)current_block + METADATA + size + FENCE_SIZE, FENCE, FENCE_SIZE);

            return (void *) ((uint8_t *)current_block + METADATA + FENCE_SIZE);
        }

        count_size += current_block->size + METADATA + 2 * FENCE_SIZE;
        last_memory_block = current_block;
        current_block = current_block->next;
    }

    // if we didn't find any block
    // we can create block at the end of list if there is enough memory
    if (count_size + METADATA + size + 2 * FENCE_SIZE <= memory_manager.memory_size) {
        current_block = (struct memory_chunk_t *)((uint8_t *) last_memory_block + last_memory_block->size + FENCE_SIZE);
        current_block->prev = last_memory_block;
        last_memory_block->next = current_block;

        current_block->next = NULL;
        current_block->size = size;
        current_block->free = 0;

        memset((uint8_t *)current_block + METADATA, FENCE, FENCE_SIZE);
        memset((uint8_t *)current_block + METADATA + size + FENCE_SIZE, FENCE, FENCE_SIZE);

        return (void *)((uint8_t *)current_block + METADATA + FENCE_SIZE);
    }

    return NULL;
}

void memory_free(void *address) {
    if (address == NULL || memory_manager.first_memory_chunk == NULL) return;

    struct memory_chunk_t *current_metadata = (struct memory_chunk_t *)((uint8_t *)address - METADATA - FENCE_SIZE);

    if (((char *)address - METADATA - FENCE_SIZE) < (char *)memory_manager.memory_start) return;

    if (validate_address(current_metadata)) return;
    if (heap_validate()) return;

    if (current_metadata->free != 0 && current_metadata->free != 1) return;
    if (current_metadata->size > memory_manager.memory_size) return;

    if (current_metadata->free == 0) {
        current_metadata->free = 1;

        if (current_metadata->next == NULL && current_metadata->prev == NULL) {
            memory_manager.first_memory_chunk = NULL;
            return;
        }

        if (current_metadata->next != NULL) {
            current_metadata->size = (char *)current_metadata->next - (char *)current_metadata + FENCE_SIZE;
        }

        // if next not null
        if (current_metadata->next != NULL && current_metadata->next->free == 1) {
            struct memory_chunk_t *next_block = current_metadata->next;
            current_metadata->next = next_block->next;

            if (next_block->next != NULL) {
                next_block->next->prev = current_metadata;
            }

            current_metadata->size = current_metadata->size + next_block->size + METADATA + 2 * FENCE_SIZE;
        }

        // if prev not null
        if (current_metadata->prev != NULL && current_metadata->prev->free == 1) {
            struct memory_chunk_t *prev_block = current_metadata->prev;

            prev_block->next = current_metadata->next;
            prev_block->size = prev_block->size + current_metadata->size;

            if (current_metadata->next != NULL) {
                current_metadata->next->prev = prev_block;
            }

            current_metadata = prev_block;

            if (current_metadata->next == NULL && current_metadata->prev != NULL) {
                current_metadata->prev->next = NULL;
            }
        }

        if (current_metadata->next == NULL && current_metadata->prev == NULL) {
            memory_manager.first_memory_chunk = NULL;
            return;
        }
    }
}

int heap_validate(void) {
    struct memory_chunk_t *memory_block = memory_manager.first_memory_chunk;
    if (memory_block == NULL) return 0;

    while (memory_block) {
        if (memory_block->free == 0) {
            // before data
            char *data = (char *) memory_block + METADATA;

            if (strncmp("xxxxxxxxxxxxxxxx", data, FENCE_SIZE) !=0) {
                return 1;
            }

            // after data
            data = (char *) memory_block + METADATA + FENCE_SIZE + memory_block->size;
            if (strncmp("xxxxxxxxxxxxxxxx", data, FENCE_SIZE) != 0) {
                return 1;
            }
        }
        memory_block = memory_block->next;
    }

    return 0;
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
