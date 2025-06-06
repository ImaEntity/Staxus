#ifndef HH_MEMALLOC
#define HH_MEMALLOC

#include "map.h"
#include "../types.h"

typedef struct _BlockHeader BlockHeader;
struct _BlockHeader {
    u64 size;
    BlockHeader *next;
    boolean inUse;
    u8 flags;
};

u64   InitializeMemory(MemoryMap *memory);
void *BlkAlloc(u64 size);
void  BlkFree(void *ptr);

#endif