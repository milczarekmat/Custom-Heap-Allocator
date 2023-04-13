#ifndef HEAP_PROJECT_TO_GIT_HEAP_H
#define HEAP_PROJECT_TO_GIT_HEAP_H

#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#define FENCE_SIZE 8

struct memory_manager_t
{
    void *memory_start;
    size_t memory_size;
    struct memory_chunk_t *first_memory_chunk;
};

struct memory_chunk_t
{
    struct memory_chunk_t* prev;
    struct memory_chunk_t* next;
    size_t size;
    int free;
    int hash;
};

int heap_setup(void);
void heap_clean(void);

#endif
