#include "../types.h"
#include "alloc.h"
#include "map.h"

static BlockHeader *firstBlock = NULL;

u64 InitializeMemory(MemoryMap *memory) {
    u64 blockCount = 0;
    BlockHeader *lastBlock = NULL;
    for(u64 i = 0; i < memory -> mapSize / memory -> descriptorSize; i++) {
        byte *blocks = (byte *) memory -> map;
        MemoryDescriptor block = *(MemoryDescriptor *) (blocks + i * memory -> descriptorSize);

        if(block.type != EfiConventionalMemory) continue;

        BlockHeader *header = (BlockHeader *) block.physAddr;

        header -> size = block.numPages * 4096;
        header -> next = NULL;
        header -> inUse = false;
        header -> flags = 0;

        if(lastBlock != NULL) lastBlock -> next = header;
        if(firstBlock == NULL) firstBlock = header;

        lastBlock = header;
        blockCount++;
    }

    return blockCount;
}

void *BlkAlloc(u64 size) {
    BlockHeader *current = firstBlock;

    while(current != NULL) {
        if(current -> size < size || current -> inUse) {
            current = current -> next;
            continue;
        }

        if(current -> size > size + sizeof(BlockHeader)) {
            BlockHeader *newHeader = (BlockHeader *) ((byte *) current + sizeof(BlockHeader) + size);

            newHeader -> next = current -> next;
            newHeader -> size = current -> size - size - sizeof(BlockHeader);
            newHeader -> inUse = false;

            current -> next = newHeader;
            current -> size = size;
        }

        current -> inUse = true;
        return ((byte *) current + sizeof(BlockHeader));
    }

    return NULL;
}

void *BlkRealloc(void *ptr, u64 size) {
    BlockHeader *header = (BlockHeader *) ((byte *) ptr - sizeof(BlockHeader));
    if(header -> size >= size) return ptr;

    u64 sizeDif = size - header -> size;
    if(
        header -> next != NULL &&
        !header -> next -> inUse &&
        header -> next -> size + sizeof(BlockHeader) >= sizeDif
    ) {
        header -> size += header -> next -> size + sizeof(BlockHeader);
        header -> next = header -> next -> next;
        return ptr;
    }

    void *newPtr = BlkAlloc(size);
    if(newPtr == NULL) return NULL;

    for(u64 i = 0; i < header -> size; i++)
        ((byte *) newPtr)[i] = ((byte *) ptr)[i];

    BlkFree(ptr);
    return newPtr;
}

void BlkFree(void *ptr) {
    BlockHeader *header = (BlockHeader *) ((byte *) ptr - sizeof(BlockHeader));

    header -> inUse = false;

    // merge with next block if it's also free
    if(header -> next != NULL && !header -> next -> inUse) {
        header -> size += header -> next -> size + sizeof(BlockHeader);
        header -> next = header -> next -> next;
    }

    BlockHeader *current = firstBlock;
    while(current != NULL) {
        if(current -> next == header)
            break;
    }

    if(current == NULL) return;

    // merge with prev block if it's also free
    if(!current -> next -> inUse) {
        current -> size += header -> size + sizeof(BlockHeader);
        current -> next = header -> next;
    }
}