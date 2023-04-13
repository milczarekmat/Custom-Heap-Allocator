#include "heap.h"

struct memory_manager_t memory_manager;

int heap_setup(void) {
    void *heap = sbrk(0);
    if (!heap) {
        return -1;
    }
    memory_manager.memory_size = 0;
    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_start = heap;
    return 0;
}

void heap_clean(void) {
    sbrk(memory_manager.memory_size * -1);
    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_start = NULL;
    memory_manager.memory_size = 0;
}

void *heap_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    if (heap_validate()) {
        return NULL;
    }

    if (memory_manager.first_memory_chunk == NULL) {
        if (size + sizeof(struct memory_chunk_t) + FENCE_SIZE * 2 > memory_manager.memory_size) {
            void *temp = sbrk(size + 2 * FENCE_SIZE + sizeof(struct memory_chunk_t));
            if (errno == ENOMEM) {
                errno = 0;
                return NULL;
            }
            memory_manager.first_memory_chunk = (struct memory_chunk_t *) temp;
            memory_manager.memory_size += size + 2 * FENCE_SIZE + sizeof(struct memory_chunk_t);
            memory_manager.first_memory_chunk->size = size + 2 * FENCE_SIZE;
            memory_manager.first_memory_chunk->free = 1;
            memory_manager.first_memory_chunk->next = NULL;
            memory_manager.first_memory_chunk->prev = NULL;
            hash_control_structures();
            return heap_malloc(size);
        }
        memory_manager.first_memory_chunk = (struct memory_chunk_t *) memory_manager.memory_start;
        memory_manager.first_memory_chunk->prev = NULL;
        memory_manager.first_memory_chunk->next = NULL;
        memory_manager.first_memory_chunk->size = size + 2 * FENCE_SIZE;
        memory_manager.first_memory_chunk->free = 0;
        hash_control_structures();
        void *ptr = (uint8_t *) memory_manager.first_memory_chunk + sizeof(struct memory_chunk_t) + FENCE_SIZE;
        char *fence = (char *) memory_manager.first_memory_chunk + sizeof(struct memory_chunk_t);
        memset(fence, '#', FENCE_SIZE);
        memset(fence + size + FENCE_SIZE, '#', FENCE_SIZE);
        return ptr;
    }
    struct memory_chunk_t *p_chunk = NULL;
    struct memory_chunk_t *p_last = NULL;
    int size_used = 0;

    for (struct memory_chunk_t *p_current = memory_manager.first_memory_chunk; p_current != NULL;
         p_current = p_current->next) {

        if ((size + 2 * FENCE_SIZE <= p_current->size) && p_current->free) {
            p_chunk = p_current;
            break;
        }
        if (p_current->next == NULL) {
            p_last = p_current;
        }
        if (p_current->prev) {
            size_used += (int) ((uint8_t *) p_current - (uint8_t *) p_current->prev);
        }
    }

    if (!p_chunk) {
        size_used += (int) (sizeof(struct memory_chunk_t) + p_last->size);
        int memory_left = (int) (memory_manager.memory_size - size_used);
        if (memory_left >= (int) (size + sizeof(struct memory_chunk_t) + FENCE_SIZE * 2)) {
            struct memory_chunk_t *p_new = (struct memory_chunk_t *) ((uint8_t *) p_last
                                                                      + sizeof(struct memory_chunk_t) + p_last->size);
            p_new->prev = p_last;
            p_last->next = p_new;
            p_new->next = NULL;
            p_chunk = p_new;
        } else {
            void *temp = sbrk(size + 2 * FENCE_SIZE + sizeof(struct memory_chunk_t));
            if (errno == ENOMEM) {
                errno = 0;
                return NULL;
            }
            assert(p_last != temp);
            memory_manager.memory_size += size + 2 * FENCE_SIZE + sizeof(struct memory_chunk_t);
            p_last->next = (struct memory_chunk_t *) temp;
            p_last->next->size = size + 2 * FENCE_SIZE;
            p_last->next->free = 1;
            p_last->next->next = NULL;
            p_last->next->prev = p_last;
            hash_control_structures();

            return heap_malloc(size);
        }
    }
    p_chunk->free = 0;
    p_chunk->size = size + 2 * FENCE_SIZE;
    hash_control_structures(p_chunk);

    void *ptr = (uint8_t *) p_chunk + sizeof(struct memory_chunk_t) + FENCE_SIZE;
    char *fence = (char *) p_chunk + sizeof(struct memory_chunk_t);
    memset(fence, '#', FENCE_SIZE);
    memset(fence + size + FENCE_SIZE, '#', FENCE_SIZE);
    return ptr;
}

void hash_control_structures() {
    for (struct memory_chunk_t *p_current = memory_manager.first_memory_chunk;
         p_current != NULL; p_current = p_current->next) {
        p_current->hash = hash_single_structure(p_current);
    }
}

int heap_validate(void) {
    if (memory_manager.memory_start == NULL) {
        return 2;
    }

    int check_structures = check_control_structures(memory_manager.first_memory_chunk);

    if (!check_structures) {
        return 3;
    }

    for (struct memory_chunk_t *p_current = memory_manager.first_memory_chunk;
         p_current != NULL; p_current = p_current->next) {
        if (p_current->free) {
            continue;
        }
        char *p_check = (char *) p_current + sizeof(struct memory_chunk_t);
        for (int j = 0; j < 2; j++, p_check += p_current->size - 2 * FENCE_SIZE) {
            for (int i = 0; i < FENCE_SIZE; i++) {
                if (*p_check != '#') {
                    return 1;
                }
                p_check++;
            }
        }
    }
    return 0;
}

int check_control_structures() {
    for (struct memory_chunk_t *p_current = memory_manager.first_memory_chunk;
         p_current != NULL; p_current = p_current->next) {
        int new_hash = hash_single_structure(p_current);
        int *old_hash = (int *) ((uint8_t *) p_current + (sizeof(struct memory_chunk_t) - sizeof(int)));
        if (new_hash != *old_hash) {
            return 0;
        }
    }
    return 1;
}

int hash_single_structure(struct memory_chunk_t *chunk) {
    uint8_t *ptr = (uint8_t *) chunk;
    int hash_result = 0;
    for (int i = 0; i < (int) (sizeof(struct memory_chunk_t) - sizeof(int)); i++) {
        hash_result += *ptr++;
    }
    return hash_result;
}

void *heap_calloc(size_t number, size_t size) {
    if (number == 0 || size == 0) {
        return NULL;
    }
    if (heap_validate()) {
        return NULL;
    }

    void *result = heap_malloc(size * number);
    if (result) {
        memset(result, 0, size * number);
    }
    return result;
}

void heap_free(void *memblock) {
    if (!memblock) {
        return;
    }
    struct memory_chunk_t *chunk_to_free = (struct memory_chunk_t *) ((uint8_t *) memblock -
                                                                      sizeof(struct memory_chunk_t) - FENCE_SIZE);

    if (!check_if_pointer_is_block_pointer(chunk_to_free)) {
        return;
    }

    if (heap_validate()) {
        return;
    }

    int conjunction_flag = 0;
    if (chunk_to_free->next) {
        if (chunk_to_free->next->free) {
            heap_conjunction(chunk_to_free);
            conjunction_flag = 1;
        }
    }
    if (chunk_to_free->prev) {
        if (chunk_to_free->prev->free) {
            heap_conjunction(chunk_to_free->prev);
            conjunction_flag = 1;
        }
    }

    chunk_to_free->free = 1;
    if (!conjunction_flag) {
        chunk_to_free->size = calculate_full_size_of_memblock(chunk_to_free);
    }

    int check = check_if_heap_is_empty(memory_manager.first_memory_chunk);

    if (!check) {
        memory_manager.first_memory_chunk = NULL;
    }
    hash_control_structures();
}

int check_if_pointer_is_block_pointer(struct memory_chunk_t *chunk) {
    for (struct memory_chunk_t *p_current = memory_manager.first_memory_chunk;
         p_current != NULL; p_current = p_current->next) {
        if (chunk == p_current) {
            return 1;
        }
    }
    return 0;
}

void heap_conjunction(struct memory_chunk_t *chunk) {
    int size_after_conjuction = (int) (calculate_full_size_of_memblock(chunk)
                                       + calculate_full_size_of_memblock(chunk->next) + sizeof(struct memory_chunk_t));

    chunk->free = 1;
    chunk->size = size_after_conjuction;
    if (chunk->next) {
        chunk->next = chunk->next->next;
    }
    if (chunk->next) {
        chunk->next->prev = chunk;
    } else {
        if (chunk->prev) {
            chunk->prev->next = NULL;
        }
    }
}

int calculate_full_size_of_memblock(struct memory_chunk_t *chunk) {
    if (!chunk) {
        return 0;
    }
    int result;
    if (chunk->next) {
        result = (int) ((uint8_t *) chunk->next - (uint8_t *) chunk - sizeof(struct memory_chunk_t));
    } else {
        if (chunk == memory_manager.first_memory_chunk) {
            result = chunk->size + 2 * FENCE_SIZE;
        } else {
            result = rest_of_memory_in_heap(chunk);
        }
    }
    return result;
}

int rest_of_memory_in_heap(struct memory_chunk_t *chunk) {
    int result = 0;
    if (memory_manager.first_memory_chunk == chunk) {
        return result;
    }
    for (struct memory_chunk_t *p_current = memory_manager.first_memory_chunk; p_current != NULL;
         p_current = p_current->next) {
        if (p_current->prev) {
            result += (int) ((uint8_t *) p_current - (uint8_t *) p_current->prev);
        }
    }
    result += (int) (chunk->size + sizeof(struct memory_chunk_t));
    return (int) (memory_manager.memory_size - result);
}

void *heap_realloc(void *memblock, size_t count) {
    if (count == 0 && !memblock) {
        return NULL;
    }

    if (heap_validate()) {
        return NULL;
    }

    if (!memblock) {
        return heap_malloc(count);
    }

    struct memory_chunk_t *chunk = (struct memory_chunk_t *) ((uint8_t *) memblock
                                                              - sizeof(struct memory_chunk_t) - FENCE_SIZE);
    if (!check_if_pointer_is_block_pointer(chunk)) {
        return NULL;
    }

    if (chunk->size == count + 2 * FENCE_SIZE) {
        return memblock;
    } else if (chunk->size > count + 2 * FENCE_SIZE) {
        if (count == 0) {
            chunk->free = 1;
            hash_control_structures();
            return NULL;
        }
        chunk->size = count + 2 * FENCE_SIZE;
        hash_control_structures();
        char *ptr = (char *) memblock + count;
        memset(ptr, '#', FENCE_SIZE);
        return memblock;
    } else {
        if (calculate_full_size_of_memblock(chunk) - 2 * FENCE_SIZE >=
            (int) count) { // przesuniecie pamieci w tym samym bloku
            chunk->size = count + 2 * FENCE_SIZE;
            memset(memblock, 'b', count);
            char *ptr = (char *) memblock + count;
            memset(ptr, '#', FENCE_SIZE);
            hash_control_structures();
            return memblock;
        }

        if (chunk->next) {
            if (chunk->next->free) {
                size_t complete_size =
                        calculate_full_size_of_memblock(chunk) + calculate_full_size_of_memblock(chunk->next) -
                        2 * FENCE_SIZE +
                        sizeof(struct memory_chunk_t);
                if (complete_size >= count) { //przeniesienie do nastepnego bloku/obszaru
                    heap_free(memblock);
                    void *ptr = heap_malloc(count);
                    if (ptr) {
                        return ptr;
                    }
                } else {
                    if (!chunk->next->next) { //dodanie pamieci do drugiego wolnego bloku
                        int new_size = (int) (count - complete_size);
                        void *temp = sbrk(new_size);
                        if (!temp) {
                            return NULL;
                        }
                        memory_manager.memory_size += new_size;
                        chunk->size = count + 2 * FENCE_SIZE;
                        chunk->next = chunk->next->next;
                        hash_control_structures();
                        memset((uint8_t *) memblock + count, '#', FENCE_SIZE);
                        return memblock;
                    } else {
                        void *new_mem = heap_malloc(count);
                        if (!new_mem) {
                            return NULL;
                        }
                        memcpy(new_mem, memblock, chunk->size);
                        heap_free(memblock);
                        return new_mem;
                    }
                }
            } else { // przeniesienie bloku do innego miejsca w pamieci
                void *new_mem = heap_malloc(count);
                if (!new_mem) {
                    return NULL;
                }
                memcpy(new_mem, memblock, chunk->size - 2 * FENCE_SIZE);
                heap_free(memblock);
                return new_mem;
            }
        }

        //ostatni blok na stercie i/lub brak pamieci
        int new_size = (int) ((count + FENCE_SIZE) - rest_of_memory_in_heap(chunk) - (chunk->size - FENCE_SIZE));
        void *temp = sbrk(new_size);
        if (errno == ENOMEM) {
            errno = 0;
            return NULL;
        }
        memory_manager.memory_size += new_size;
        chunk->size = count + 2 * FENCE_SIZE;
        hash_control_structures();
        memset((uint8_t *) temp + new_size - FENCE_SIZE, '#', FENCE_SIZE);
        return memblock;

    }
}
