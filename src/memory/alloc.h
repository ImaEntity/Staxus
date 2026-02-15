#ifndef HH_MEM_ALLOC
#define HH_MEM_ALLOC

#include "map.h"
#include <types.h>

typedef struct {
    void  (*init)   (MemoryMap *memory, void *kernel_entry);
    void *(*malloc) (u64 size);
    void *(*aalloc) (u64 size, u64 align);
    void *(*realloc)(void *ptr, u64 size);
    void  (*free)   (void *ptr);
    void  (*cleanup)();

    u64 (*get_usable)();
    u64 (*get_available)();
} MemoryManager;

boolean LoadMemoryManager(MemoryManager manager, MemoryMap *map, void *kernel_entry);

void *malloc(u64 size);
void *aalloc(u64 size, u64 align); // Aligned alloc
void *realloc(void *ptr, u64 size);
void  free(void *ptr);

u64 GetUsableMemory();
u64 GetAvailableMemory();

#endif