#include "alloc.h"
#include <types.h>

#include <video/print.h>

static MemoryManager gManager = {0};
boolean LoadMemoryManager(MemoryManager manager, MemoryMap *map, void *kernel_entry) {
    if(gManager.cleanup != NULL)
        gManager.cleanup();

    gManager = manager;

    if(gManager.init == NULL) return false;
    gManager.init(map, kernel_entry);

    return true;
}


void *malloc(u64 size) {
    if(gManager.malloc == NULL) return NULL;
    return gManager.malloc(size);
}

void *aalloc(u64 size, u64 align) {
    if(gManager.aalloc == NULL) return NULL;
    return gManager.aalloc(size, align);
}

void *realloc(void *ptr, u64 size) {
    if(gManager.realloc == NULL) return NULL;
    return gManager.realloc(ptr, size);
}

void free(void *ptr) {
    if(gManager.free == NULL) return;
    gManager.free(ptr);
}


u64 GetAvailableMemory() {
    if(gManager.get_available == NULL) return -1;
    return gManager.get_available();
}

u64 GetUsableMemory() {
    if(gManager.get_usable == NULL) return -1;
    return gManager.get_usable();
}