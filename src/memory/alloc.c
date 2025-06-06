#include "../types.h"
#include "alloc.h"
#include "map.h"

static BlockHeader *firstBlock = NULL;

u64 InitializeMemory(MemoryMap *memory) {
    u64 blockCount = 0;
    BlockHeader *lastBlock = NULL;
    firstBlock = NULL;
    
    for(u64 i = 0; i < memory -> mapSize / memory -> descriptorSize; i++) {
        byte *blocks = (byte *) memory -> map;
        MemoryDescriptor block = *(MemoryDescriptor *) (blocks + i * memory -> descriptorSize);

        if(block.type != EfiConventionalMemory) continue;

        BlockHeader *header = (BlockHeader *) (
            block.virtAddr != 0 ?
                block.virtAddr :
                block.physAddr
        );

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
            newHeader -> flags = 0;

            current -> next = newHeader;
            current -> size = size;
        }

        current -> inUse = true;
        return ((byte *) current + sizeof(BlockHeader));
    }

    return NULL;
}

void *BlkRealloc(void *ptr, u64 size) {
    if(ptr == NULL) return BlkAlloc(size);
    if(size == 0) {
        BlkFree(ptr);
        return NULL;
    }

    BlockHeader *header = (BlockHeader *) ((byte *) ptr - sizeof(BlockHeader));
    if(header -> size >= size) return ptr;

    u64 oldSize = header -> size;
    u64 sizeDif = size - header -> size;

    if(
        header -> next != NULL &&
        !header -> next -> inUse &&
        header -> next -> size + sizeof(BlockHeader) >= sizeDif
    ) {
        header -> size += header -> next -> size + sizeof(BlockHeader);
        header -> next = header -> next -> next;

        if(header -> size > size + sizeof(BlockHeader)) {
            BlockHeader *newHeader = (BlockHeader *) ((byte *) header + sizeof(BlockHeader) + size);

            newHeader -> next = header -> next;
            newHeader -> size = header -> size - size - sizeof(BlockHeader);
            newHeader -> inUse = false;
            newHeader -> flags = 0;

            header -> next = newHeader;
            header -> size = size;
        }

        return ptr;
    }

    void *newPtr = BlkAlloc(size);
    if(newPtr == NULL) return NULL;

    for(u64 i = 0; i < oldSize; i++)
        ((byte *) newPtr)[i] = ((byte *) ptr)[i];

    BlkFree(ptr);
    return newPtr;
}

void BlkFree(void *ptr) {
    if(ptr == NULL) return;
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
        
        current = current -> next;
    }

    if(current == NULL) return;

    // merge with prev block if it's also free
    if(!current -> inUse) {
        current -> size += header -> size + sizeof(BlockHeader);
        current -> next = header -> next;
    }
}