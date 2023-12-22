#include <stddef.h>
#include <string.h>
#include "heap.h"
#include "custom_unistd.h"

#define METADATA sizeof(struct memory_chunk_t)
#define FENCE_SIZE 4
#define TWO_FENCES 8
#define SBRK_ERROR (void *) - 1

struct memory_manager_t memory_manager;

char fence[] = "####";

int heap_setup(void) {
    void *memory = custom_sbrk(0);

    if (memory == SBRK_ERROR) return -1;

    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_size = 0;
    memory_manager.memory_start = memory;

    return 0;
}

void heap_clean(void) {
    custom_sbrk((long) - memory_manager.memory_size);

    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_start = NULL;
    memory_manager.memory_size = 0;
}

void *heap_malloc(size_t size){
    if (size == 0) return NULL;
    if (memory_manager.memory_start == NULL) return NULL;

    char *data;

    struct memory_chunk_t *current_block = memory_manager.first_memory_chunk;

    if (current_block == NULL){
        current_block = (struct memory_chunk_t*)custom_sbrk((long)(METADATA + size + TWO_FENCES));
        if (current_block == SBRK_ERROR) return NULL;

        current_block->prev = NULL;
        current_block->next = NULL;
        current_block->size = size;
        current_block->free = 0;

        memory_manager.memory_size += METADATA + size + TWO_FENCES;
        memory_manager.first_memory_chunk = current_block;

        data = (char *)current_block + METADATA;
        memset(data, '#', FENCE_SIZE);

        data = (char *)current_block + METADATA + FENCE_SIZE + current_block->size;
        memset(data, '#', FENCE_SIZE);

        current_block->metadata = calculate_metadata(memory_manager.first_memory_chunk);

        return (void*)((char *)current_block + METADATA + FENCE_SIZE);
    }

    if (heap_validate() != 0) return NULL;

    while (current_block) {
        if (current_block->free == 1 && current_block->size >= size + TWO_FENCES) {
            current_block->size = size;
            current_block->free = 0;

            data = (char *)current_block + METADATA;
            memset(data, '#', FENCE_SIZE);

            data = (char *)current_block + METADATA + FENCE_SIZE + current_block->size;
            memset(data, '#', FENCE_SIZE);

            current_block->metadata = calculate_metadata(current_block);

            return (void *) ((char *)current_block + METADATA + FENCE_SIZE);
        }

        if (current_block->next == NULL) {
            size_t needed_size = METADATA + size + TWO_FENCES;

            struct memory_chunk_t *new_memory_block = (struct memory_chunk_t*)custom_sbrk(needed_size);
            if (new_memory_block == SBRK_ERROR) return NULL;

            memory_manager.memory_size += needed_size;

            current_block->next = new_memory_block;
            new_memory_block->prev = current_block;
            new_memory_block->next = NULL;

            new_memory_block->size = size;
            new_memory_block->free = 0;

            data = (char *)new_memory_block + METADATA;
            memset(data, '#', FENCE_SIZE);

            data = (char *)new_memory_block + METADATA + FENCE_SIZE + new_memory_block->size;
            memset(data, '#', FENCE_SIZE);

            current_block->metadata = calculate_metadata(current_block);
            new_memory_block->metadata = calculate_metadata(new_memory_block);

            return (void *) ((char *)new_memory_block + METADATA + FENCE_SIZE);
        }

        current_block = current_block->next;
    }

    return NULL;
}

void* heap_calloc(size_t number, size_t size) {
    if (number == 0) return NULL;
    if (size == 0) return NULL;

    int memory_size = number * size;

    void *allocate_memory = heap_malloc(memory_size);
    if (allocate_memory == NULL) return NULL;

    for (int i = 0; i < memory_size; ++i) {
        *(char*)((char *)allocate_memory + i) = 0;
    }

    return allocate_memory;
}

int heap_validate(void) {
    if (memory_manager.memory_start == NULL) return 2;

    struct memory_chunk_t *memory_block = memory_manager.first_memory_chunk;

    while (memory_block) {
        int metadata = calculate_metadata(memory_block);
        if (memory_block->metadata != metadata) return 3;

        if (memory_block->free == 0) {
            char *data = (char *) memory_block + METADATA;

            if (strncmp(fence, data, FENCE_SIZE) != 0) {
                return 1;
            }

            data = (char *) memory_block + METADATA + FENCE_SIZE + memory_block->size;
            if (strncmp(fence, data, FENCE_SIZE) != 0) {
                return 1;
            }
        }
        memory_block = memory_block->next;
    }
    return 0;
}

void* heap_realloc(void* memblock, size_t count) {
    if (memory_manager.memory_start == NULL) return NULL;
    if (heap_validate() != 0) return NULL;
    if (memblock == NULL) return heap_malloc(count);

    if (get_pointer_type(memblock) != pointer_valid) return NULL;

    struct memory_chunk_t* memory_block = (struct memory_chunk_t*)((char *)memblock - METADATA - FENCE_SIZE);
    struct memory_chunk_t* memory_block_next = memory_block->next;

    if (count == 0) {
        heap_free(memblock);
        return NULL;
    }

    if (memory_block->size == count) return memblock;

    if (count < memory_block->size) {
        memory_block->size = count;

        char *data = (char *)memory_block + METADATA + FENCE_SIZE + memory_block->size;
        memset(data, '#', FENCE_SIZE);

        memory_block->metadata = calculate_metadata(memory_block);

        return memblock;
    }

    if (count > memory_block->size) {
        if (memory_block_next != NULL) {
            char *next_block = (char*)memory_block_next + METADATA + memory_block_next->size + TWO_FENCES;
            char *curr_block = (char*)memory_block;

            size_t space_between_blocks = (next_block - curr_block) - METADATA - TWO_FENCES;

            if (space_between_blocks >= count) {
                memory_block_next->prev = memory_block;
                memory_block->next = memory_block_next->next;

                memory_block->size = count;

                char *data = (char *)memory_block + METADATA + FENCE_SIZE + memory_block->size;
                memset(data, '#', FENCE_SIZE);

                memory_block->metadata = calculate_metadata(memory_block);

                return memblock;
            }

            if (space_between_blocks < count) {
                void * new_memory_block = heap_malloc(count);
                if(new_memory_block == NULL) return NULL;

                memcpy((char*)new_memory_block, (char*)memory_block + METADATA + FENCE_SIZE, memory_block->size);

                heap_free((char*)memory_block + METADATA + FENCE_SIZE);

                return new_memory_block;
            }
        }

        if (memory_block_next == NULL) {
            size_t needed_size = count - memory_block->size;

            void *get_more_memory = custom_sbrk(needed_size);
            if (get_more_memory == SBRK_ERROR) return NULL;

            memory_block->size = count;
            memory_manager.memory_size += needed_size;

            char *data = (char *)memory_block + METADATA + FENCE_SIZE + memory_block->size;
            memset(data, '#', FENCE_SIZE);

            memory_block->metadata = calculate_metadata(memory_block);

            return memblock;
        }
    }
    return NULL;
}

void heap_free(void* memblock) {
    if (memblock == NULL) return;
    if (memory_manager.memory_start == NULL) return;
    if (memory_manager.first_memory_chunk == NULL) return;
    if (heap_validate() != 0) return;

    struct memory_chunk_t * current_block = (struct memory_chunk_t*)((char*)memblock - METADATA - FENCE_SIZE);

    if (current_block->free != 0 && current_block->free != 1) return;
    if (current_block->size > memory_manager.memory_size) return;

    if (current_block->free == 0){
        current_block->free = 1;

        if (current_block->next == NULL && current_block->prev == NULL){
            memory_manager.first_memory_chunk = NULL;
            return;
        }

        if (current_block->next != NULL){
            char *start_memory_block = (char *)current_block + METADATA;
            char *end_memory_block = (char *)current_block->next;

            current_block->size = end_memory_block - start_memory_block;
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

        current_block = memory_manager.first_memory_chunk;
        while (current_block){
            current_block->metadata = calculate_metadata(current_block);
            current_block = current_block->next;
        }
    }
}

size_t heap_get_largest_used_block_size(void) {
    if (memory_manager.memory_start == NULL) return 0;
    if (memory_manager.first_memory_chunk == NULL) return 0;
    if (heap_validate() != 0) return 0;

    struct memory_chunk_t * current_block = memory_manager.first_memory_chunk;
    size_t max_size = 0;

    while (current_block) {
        if (current_block->free == 0 && current_block->size > max_size) {
            max_size = current_block->size;
        }
        current_block = current_block->next;
    }

    return max_size;
}

enum pointer_type_t get_pointer_type(const void* const pointer) {
    if (pointer == NULL) return pointer_null;
    if (memory_manager.first_memory_chunk == NULL) return pointer_unallocated;
    if (heap_validate() !=0) return pointer_heap_corrupted;

    struct memory_chunk_t* current_memory_block = memory_manager.first_memory_chunk;
    while(current_memory_block != NULL) {
        if(current_memory_block->free == 0) {
            if ((char *) current_memory_block + METADATA > (char *) pointer)return pointer_control_block;
            else if (((char *) current_memory_block + METADATA + FENCE_SIZE > (char *) pointer)) return pointer_inside_fences;
            else if ((char *) current_memory_block + METADATA + FENCE_SIZE == (char *) pointer) return pointer_valid;
            else if ((char *) current_memory_block + METADATA + FENCE_SIZE + current_memory_block->size > (char *) pointer) return pointer_inside_data_block;
            else if (((char *) current_memory_block + METADATA + TWO_FENCES + current_memory_block->size > (char *) pointer)) return pointer_inside_fences;
            else if(current_memory_block->next != NULL && (char*)current_memory_block->next > (char*)pointer && ((char *) current_memory_block + METADATA + TWO_FENCES + current_memory_block->size <= (char *) pointer)) return pointer_unallocated;
        }
        else if(((char *) current_memory_block + METADATA + current_memory_block->size > (char *) pointer)) return pointer_unallocated;

        current_memory_block = current_memory_block->next;
    }
    return pointer_unallocated;
}

int calculate_metadata(struct memory_chunk_t *memory_block) {
    if (memory_manager.memory_start == NULL) return 0;
    if (memory_manager.first_memory_chunk == NULL) return 0;
    if (memory_block == NULL) return 0;

    int check_metadata = 0;
    char curr_bit;
    int metadata_size = METADATA - 4;

    for (int i = 0;i < metadata_size; i++) {
        curr_bit = *((char *)((char *)memory_block + i * sizeof(char)));
        check_metadata = check_metadata + curr_bit;
    }
    return check_metadata;
}

