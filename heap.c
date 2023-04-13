#include "heap.h"

struct memory_manager_t memory_manager;

int heap_setup(void){
    void *heap = sbrk(0);
    if (!heap){
        return -1;
    }
    memory_manager.memory_size = 0;
    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_start = heap;
    return 0;
}

void heap_clean(void){
    sbrk(memory_manager.memory_size * -1);
    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_start = NULL;
    memory_manager.memory_size = 0;
}

void* heap_malloc(size_t size){
    if(size == 0){
        return NULL;
    }

    if (heap_validate()){
        return NULL;
    }

    if (memory_manager.first_memory_chunk == NULL){
        if (size + sizeof(struct memory_chunk_t) + FENCE_SIZE*2 > memory_manager.memory_size){
            void *temp = sbrk(size + 2*FENCE_SIZE + sizeof(struct memory_chunk_t));
            if (errno == ENOMEM){
                errno = 0;
                return NULL;
            }
            memory_manager.first_memory_chunk = (struct memory_chunk_t*)temp;
            memory_manager.memory_size += size + 2*FENCE_SIZE + sizeof(struct memory_chunk_t);
            memory_manager.first_memory_chunk->size = size + 2*FENCE_SIZE;
            memory_manager.first_memory_chunk->free = 1;
            memory_manager.first_memory_chunk->next = NULL;
            memory_manager.first_memory_chunk->prev = NULL;
            hash_control_structures();
            return heap_malloc(size);
        }
        memory_manager.first_memory_chunk = (struct memory_chunk_t*)memory_manager.memory_start;
        memory_manager.first_memory_chunk->prev = NULL;
        memory_manager.first_memory_chunk->next = NULL;
        memory_manager.first_memory_chunk->size = size + 2*FENCE_SIZE;
        memory_manager.first_memory_chunk->free = 0;
        hash_control_structures();
        void * ptr = (uint8_t *)memory_manager.first_memory_chunk + sizeof(struct memory_chunk_t) + FENCE_SIZE;
        char *fence = (char *)memory_manager.first_memory_chunk + sizeof(struct memory_chunk_t);
        memset(fence, '#', FENCE_SIZE);
        memset(fence + size + FENCE_SIZE, '#', FENCE_SIZE);
        return ptr;
    }
    struct memory_chunk_t* p_chunk = NULL;
    struct memory_chunk_t* p_last = NULL;
    int size_used = 0;

    for (struct memory_chunk_t* p_current = memory_manager.first_memory_chunk; p_current != NULL;
         p_current = p_current->next){

        if ((size + 2*FENCE_SIZE <= p_current->size) && p_current->free){
            p_chunk = p_current;
            break;
        }
        if (p_current->next == NULL){
            p_last = p_current;
        }
        if (p_current->prev){
            size_used += (int)((uint8_t*)p_current - (uint8_t*)p_current->prev);
        }
    }

    if (!p_chunk){
        size_used += (int)(sizeof(struct memory_chunk_t) + p_last->size);
        int memory_left = (int)(memory_manager.memory_size - size_used);
        if (memory_left >= (int)(size + sizeof(struct memory_chunk_t) + FENCE_SIZE*2)){
            struct memory_chunk_t * p_new = (struct memory_chunk_t *) ((uint8_t*)p_last
                                                                       + sizeof(struct memory_chunk_t) + p_last->size);
            p_new->prev = p_last;
            p_last->next = p_new;
            p_new->next = NULL;
            p_chunk = p_new;
        }
        else{
            void *temp = sbrk(size + 2*FENCE_SIZE + sizeof(struct memory_chunk_t));
            if (errno == ENOMEM){
                errno = 0;
                return NULL;
            }
            assert(p_last != temp);
            memory_manager.memory_size += size + 2*FENCE_SIZE + sizeof(struct memory_chunk_t);
            p_last->next = (struct memory_chunk_t*)temp;
            p_last->next->size = size + 2*FENCE_SIZE;
            p_last->next->free = 1;
            p_last->next->next = NULL;
            p_last->next->prev = p_last;
            hash_control_structures();

            return heap_malloc(size);
        }
    }
    p_chunk->free = 0;
    p_chunk->size = size + 2*FENCE_SIZE;
    hash_control_structures(p_chunk);

    void * ptr = (uint8_t *)p_chunk + sizeof(struct memory_chunk_t) + FENCE_SIZE;
    char *fence = (char *)p_chunk + sizeof(struct memory_chunk_t);
    memset(fence, '#', FENCE_SIZE);
    memset(fence + size +FENCE_SIZE, '#', FENCE_SIZE);
    return ptr;
}

void hash_control_structures(){
    for (struct memory_chunk_t* p_current = memory_manager.first_memory_chunk;
         p_current != NULL; p_current = p_current->next) {
        uint8_t *ptr = (uint8_t *) p_current;
        int hash_result = 0;
        for (int i = 0; i < (int) (sizeof(struct memory_chunk_t) - sizeof(int)); i++) {
            hash_result += *ptr++;
        }
        p_current->hash = hash_result;
    }
}

int heap_validate(void){
    if (memory_manager.memory_start == NULL){
        return 2;
    }

    int check_structures = check_control_structures(memory_manager.first_memory_chunk);

    if (!check_structures){
        return 3;
    }

    for (struct memory_chunk_t* p_current = memory_manager.first_memory_chunk;
         p_current != NULL; p_current = p_current->next){
        if (p_current->free){
            continue;
        }
        char *p_check = (char*)p_current + sizeof(struct memory_chunk_t);
        for (int j=0; j<2; j++, p_check += p_current->size-2*FENCE_SIZE){
            for (int i=0; i<FENCE_SIZE; i++){
                if (*p_check != '#'){
                    return 1;
                }
                p_check++;
            }
        }
    }
    return 0;
}