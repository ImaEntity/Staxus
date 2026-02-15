#include <types.h>

#include <memory/alloc.h>
#include <memory/utils.h>
#include <memory/map.h>

#define MEM_IN_USE  0b00000001
#define MEM_ALIGNED 0b00000010

#define MemInUse(blk) ((blk -> flags & MEM_IN_USE) != 0)

typedef struct _BlockHeader BlockHeader;
struct _BlockHeader {
    u64 size;
    BlockHeader *next;
    // boolean inUse;
    u8 flags;
};

static BlockHeader *firstBlock = NULL;
static u64 totalUsable = 0;
void idalloc_init(MemoryMap *memory, void *kernel_entry) {
    u64 blockCount = 0;
    BlockHeader *lastBlock = NULL;

    firstBlock = NULL;
    totalUsable = 0;
    
    for(u64 i = 0; i < memory -> mapSize / memory -> descriptorSize; i++) {
        byte *blocks = (byte *) memory -> map;
        MemoryDescriptor block = *(MemoryDescriptor *) (blocks + i * memory -> descriptorSize);

        if(block.type != EFI_ConventionalMemory) continue;

        u64 start = (u64) kernel_entry;
        u64 end   = (u64) kernel_entry + 4 * 1024 * 1024;

        if(block.physAddr >= start && block.physAddr <= end) continue;
        if(block.physAddr < 4 * 1024 * 1024) continue;

        BlockHeader *header = (BlockHeader *) (block.physAddr);

        header -> size = block.numPages * 4096;
        totalUsable += header -> size;

        header -> next = NULL;
        header -> flags = 0;

        if(lastBlock != NULL) lastBlock -> next = header;
        if(firstBlock == NULL) firstBlock = header;

        lastBlock = header;
        blockCount++;
    }
}

void *idalloc_malloc(u64 size) {
    if(size == 0) return NULL;
    BlockHeader *current = firstBlock;

    while(current != NULL) {
        if(current -> size < size || MemInUse(current)) {
            current = current -> next;
            continue;
        }

        if(current -> size > size + sizeof(BlockHeader)) {
            BlockHeader *newHeader = (BlockHeader *) ((byte *) current + sizeof(BlockHeader) + size);

            newHeader -> next = current -> next;
            newHeader -> size = current -> size - size - sizeof(BlockHeader);
            newHeader -> flags = 0;

            current -> next = newHeader;
            current -> size = size;
        }

        current -> flags |= MEM_IN_USE;
        return ((byte *) current + sizeof(BlockHeader));
    }

    return NULL;
}

// Aligned alloc
void *idalloc_aalloc(u64 size, u64 align) {
    if(size == 0) return NULL;
    if(align < 2) return malloc(size);

    BlockHeader *current = firstBlock;
    BlockHeader *prev = NULL;

    while(current != NULL) {
        if(current -> size < size || MemInUse(current)) {
            prev = current;
            current = current -> next;
            continue;
        }

        byte *memStart = (byte *) current + sizeof(BlockHeader);
        u64 alignment = ((u64) memStart) % align;

        if(alignment != 0) {
            u64 padding = align - alignment;

            // not enough space for another block, make more
            while(padding <= sizeof(BlockHeader))
                padding += align;

            if(current -> size < size + padding) {
                prev = current;
                current = current -> next;
                continue;
            }

            BlockHeader *alignedBlock = (BlockHeader *) ((byte *) current + padding);
            alignedBlock -> size  = current -> size - padding;
            alignedBlock -> next  = current -> next;
            alignedBlock -> flags = 0;

            current -> size = padding - sizeof(BlockHeader);
            current -> next = alignedBlock;

            if(prev != NULL && !MemInUse(prev)) {
                prev -> size += current -> size + sizeof(BlockHeader);
                prev -> next = alignedBlock;
            }

            current = alignedBlock;
            memStart = (byte *) current + sizeof(BlockHeader);
        }

        if(current -> size > size + sizeof(BlockHeader)) {
            BlockHeader *newHeader = (BlockHeader *) ((byte *) current + sizeof(BlockHeader) + size);

            newHeader -> next = current -> next;
            newHeader -> size = current -> size - size - sizeof(BlockHeader);
            newHeader -> flags = 0;

            current -> next = newHeader;
            current -> size = size;
        }

        current -> flags |= MEM_IN_USE;
        return memStart;
    }

    return NULL;
}

void *idalloc_realloc(void *ptr, u64 size) {
    if(ptr == NULL) return malloc(size);
    if(size == 0) {
        free(ptr);
        return NULL;
    }

    BlockHeader *header = (BlockHeader *) ((byte *) ptr - sizeof(BlockHeader));
    if(header -> size >= size) return ptr;

    u64 oldSize = header -> size;
    u64 sizeDif = size - header -> size;

    if(
        header -> next != NULL &&
        !MemInUse(header -> next) &&
        header -> next -> size + sizeof(BlockHeader) >= sizeDif
    ) {
        header -> size += header -> next -> size + sizeof(BlockHeader);
        header -> next = header -> next -> next;

        if(header -> size > size + sizeof(BlockHeader)) {
            BlockHeader *newHeader = (BlockHeader *) ((byte *) header + sizeof(BlockHeader) + size);

            newHeader -> next = header -> next;
            newHeader -> size = header -> size - size - sizeof(BlockHeader);
            newHeader -> flags = 0;

            header -> next = newHeader;
            header -> size = size;
        }

        return ptr;
    }

    void *newPtr = malloc(size);
    if(newPtr == NULL) return NULL;

    memcpy(newPtr, ptr, oldSize);
    free(ptr);

    return newPtr;
}

void idalloc_free(void *ptr) {
    if(ptr == NULL) return;
    BlockHeader *header = (BlockHeader *) ((byte *) ptr - sizeof(BlockHeader));

    header -> flags &= ~MEM_IN_USE;

    // merge with next block if it's also free
    if(header -> next != NULL && !MemInUse(header -> next)) {
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
    if(!MemInUse(current)) {
        current -> size += header -> size + sizeof(BlockHeader);
        current -> next = header -> next;
    }
}

u64 idalloc_get_usable() { return totalUsable; }
u64 idalloc_get_available() {
    BlockHeader *current = firstBlock;
    u64 used = 0;

    while(current != NULL) {
        if(!MemInUse(current))
            used += current -> size;

        current = current -> next;
    }

    return used;
}

const MemoryManager mmanager_get_identity() {
    return (MemoryManager) {
        .init    = idalloc_init,
        .malloc  = idalloc_malloc,
        .aalloc  = idalloc_aalloc,
        .realloc = idalloc_realloc,
        .free    = idalloc_free,
        .cleanup = NULL,

        .get_usable   = idalloc_get_usable,
        .get_available = idalloc_get_available
    };
}
