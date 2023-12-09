//
// Created by Sylwia Szyda on 16/11/2023.
//

#ifndef EASY_MALLOC_HEAP_H
#define EASY_MALLOC_HEAP_H

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

struct memory_manager_t memory_manager;

void memory_init(void *address, size_t size);
void *memory_malloc(size_t size);
void memory_free(void *address);
int validate_address(struct memory_chunk_t *current_metadata);

#endif //EASY_MALLOC_HEAP_H
