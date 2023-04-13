#ifndef HEAP_PROJECT_TO_GIT_HEAP_H
#define HEAP_PROJECT_TO_GIT_HEAP_H

#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#define FENCE_SIZE 8

struct memory_manager_t {
    void *memory_start;
    size_t memory_size;
    struct memory_chunk_t *first_memory_chunk;
};

struct memory_chunk_t {
    struct memory_chunk_t *prev;
    struct memory_chunk_t *next;
    size_t size;
    int free;
    int hash;
};

enum pointer_type_t {
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};

size_t heap_get_largest_used_block_size(void);

int heap_validate(void);

int heap_setup(void);

void heap_clean(void);

void *heap_malloc(size_t size);

int check_control_structures();

void hash_control_structures();

int hash_single_structure(struct memory_chunk_t *chunk);

void *heap_calloc(size_t number, size_t size);

void heap_free(void *memblock);

void heap_conjunction(struct memory_chunk_t *chunk);

int calculate_full_size_of_memblock(struct memory_chunk_t *chunk);

int rest_of_memory_in_heap(struct memory_chunk_t *chunk);

int check_if_pointer_is_block_pointer(struct memory_chunk_t *chunk);

int check_if_heap_is_empty(struct memory_chunk_t *chunk);

void *heap_realloc(void *memblock, size_t count);

enum pointer_type_t get_pointer_type(const void * pointer);

int check_if_block_of_current_fence_pointer_is_occupied(char *ptr);

int check_if_pointer_is_element_of_block(uint8_t *ptr);

int check_if_pointer_is_inside_data_block(const uint8_t *ptr);

#endif
