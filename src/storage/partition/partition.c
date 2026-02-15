#include "partition.h"
#include <types.h>

static u16 ctrlCount = 0;
static PartitionController *ctrlers[MAX_PARTITION_CONTROLLERS];
boolean RegisterPartitionController(PartitionController *prtc) {
    if(ctrlCount >= MAX_PARTITION_CONTROLLERS) return false;
    if(prtc == NULL) return false;
    if(prtc -> probe == NULL || prtc -> register_blocks == NULL)
        return false;

    ctrlers[ctrlCount++] = prtc;
    return true;
}

u32 RegisterPartitionBlocks(BlockDevice *device) {
    for(int i = 0; i < ctrlCount; i++) {
        if(!ctrlers[i] -> probe(device)) continue;
        return ctrlers[i] -> register_blocks(device);
    }

    return -1;
}

boolean readPartition(BlockDevice *device, void *buffer, u64 sector, u64 count) {
    Partition *part = (Partition *) device -> internal;
    if(sector + count > device -> sectorCount) return false;

    return ReadBlockDevice(part -> parent, buffer, sector + part -> lbaStart, count);
}

boolean writePartition(BlockDevice *device, void *buffer, u64 sector, u64 count) {
    Partition *part = (Partition *) device -> internal;
    if(sector + count > device -> sectorCount) return false;

    return WriteBlockDevice(part -> parent, buffer, sector + part -> lbaStart, count);
}

// required cause functions declared to exist in headers have different addresses
// than the actual function definition, and you need to have the functions be local
// otherwise calling the functions results in calling a data pointer that points to all zeros
void ApplyGenericPartitionData(BlockDevice *parent, BlockDevice *device) {
    device -> flags = parent -> flags & ~BLOCK_DEVICE_FLAG_PHYSICAL;
    device -> sectorSize = parent -> sectorSize;
    device -> write = writePartition;
    device -> read = readPartition;
}