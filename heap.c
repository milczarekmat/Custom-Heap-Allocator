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