#include <stddef.h>
#include <string.h>
#include "heap.h"

#define METADATA sizeof(struct memory_chunk_t)
#define FENCE_SIZE 16

char fence_before[] = "xxxxxxxxxxxxxxx";
char fence_after[] = "yyyyyyyyyyyyyyy";


void memory_init(void *address, size_t size) {
    if (address != NULL && size > 0) {
        memory_manager.memory_start = address;
        memory_manager.memory_size = size;
        memory_manager.first_memory_chunk = NULL;
    }
}

void *memory_malloc(size_t size){
    if (size <= 0) return NULL;
    if (heap_validate()) return NULL;

    char* data;

    // case when memory is empty
    struct memory_chunk_t *current_block = memory_manager.first_memory_chunk;
    if (current_block == NULL) {
        if (size + METADATA > memory_manager.memory_size) return NULL;

        current_block = (struct memory_chunk_t *) memory_manager.memory_start;
        memory_manager.first_memory_chunk = current_block;

        current_block->prev = NULL;
        current_block->next = NULL;
        current_block->size = size;
        current_block->free = 0;

        data = (char *)current_block + METADATA;
        memcpy(data, fence_before, FENCE_SIZE);

        data = (char *)current_block + METADATA + FENCE_SIZE + current_block->size;
        memcpy(data, fence_after, FENCE_SIZE);

        return (void *) ((char *)current_block + METADATA + FENCE_SIZE);
    }

    size_t counted_size = 0;
    current_block = memory_manager.memory_start;

    while (current_block) {
        counted_size += METADATA + current_block->size + 2 * FENCE_SIZE;

        if (current_block->free == 1 && current_block->size >= size + 2 * FENCE_SIZE) {
            current_block->size = size;
            current_block->free = 0;

            data = (char *)current_block + METADATA;
            memcpy(data, fence_before, FENCE_SIZE);

            data = (char *)current_block + METADATA + FENCE_SIZE + current_block->size;
            memcpy(data, fence_after, FENCE_SIZE);

            return (void *) ((char *) current_block + METADATA + FENCE_SIZE);
        }

        if (current_block->next == NULL) {
            if (counted_size + size + METADATA + 2 * FENCE_SIZE <= memory_manager.memory_size) {
                struct memory_chunk_t* prev_block = current_block;
                current_block = (struct memory_chunk_t*)((char*)prev_block + METADATA + current_block->size + 2 * FENCE_SIZE);

                prev_block->next = current_block;
                current_block->size = size;
                current_block->next = NULL;
                current_block->prev = prev_block;
                current_block->free = 0;

                data = (char *)current_block + METADATA;
                memcpy(data, fence_before, FENCE_SIZE);

                data = (char *)current_block + METADATA + FENCE_SIZE + current_block->size;
                memcpy(data, fence_after, FENCE_SIZE);

                return (void *) ((char *) current_block + METADATA + FENCE_SIZE);
            } else {
                return NULL;
            }
        }

        current_block = current_block->next;
    }

    return NULL;
}

void memory_free(void *address){
    if (address == NULL || memory_manager.first_memory_chunk == NULL) return;
    if (heap_validate()) return;

    struct memory_chunk_t * current_block = (struct memory_chunk_t*)((char*)address - METADATA - FENCE_SIZE);

    if (validate_address(current_block)) return;
    if (current_block->free != 0 && current_block->free != 1) return;
    if (current_block->size > memory_manager.memory_size) return;

    if (current_block->free == 0){
        current_block->free = 1;

        if (current_block->next == NULL && current_block->prev == NULL){
            memory_manager.first_memory_chunk = NULL;
            return;
        }

        if (current_block->next != NULL){
            char * start = (char*)current_block + METADATA;
            char * end = (char*)current_block->next;

            current_block->size = end - start;
        }

        if (current_block->prev != NULL) {
            if (current_block->prev->free == 1) {
                struct memory_chunk_t *prev_block = current_block->prev;
                prev_block->next = current_block->next;
                prev_block->size = METADATA + prev_block->size + current_block->size;

                if (current_block->next != NULL){
                    current_block->next->prev = prev_block;
                }

                current_block = prev_block;

                if (current_block->next == NULL && current_block->prev != NULL) {
                    current_block->prev->next = NULL;
                }
            }
        }

        if (current_block->next != NULL && current_block->next->free == 1) {
            struct memory_chunk_t *next_block = current_block->next;
            current_block->next = next_block->next;

            if (next_block->next != NULL){
                next_block->next->prev = current_block;
            }

            current_block->size = METADATA + next_block->size + current_block->size;
        }

        if (current_block->next == NULL && current_block->prev == NULL){
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

            if (strncmp(fence_before, data, FENCE_SIZE) !=0) {
                return 1;
            }

            // after data
            data = (char *) memory_block + METADATA + FENCE_SIZE + memory_block->size;
            if (strncmp(fence_after, data, FENCE_SIZE) != 0) {
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
