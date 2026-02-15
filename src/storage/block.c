#include "block.h"
#include <types.h>

boolean ReadBlockDevice(BlockDevice *device, void *buffer, u64 sector, u64 count) {
    if(device -> read == NULL) return false;
    return device -> read(device, buffer, sector, count);
}

boolean WriteBlockDevice(BlockDevice *device, void *buffer, u64 sector, u64 count) {
    if(device -> write == NULL) return false;
    return device -> write(device, buffer, sector, count);
}

static u16 blockDeviceCount = 0;
static BlockDevice *blockDevices[MAX_BLOCK_DEVICES];
boolean RegisterBlockDevice(BlockDevice *device) {
    if(blockDeviceCount >= MAX_BLOCK_DEVICES) return false;

    blockDevices[blockDeviceCount++] = device;
    return true;
}

BlockDevice **GetBlockDevices(u16 *count) {
    if(count != NULL) *count = blockDeviceCount;
    return blockDevices;
}