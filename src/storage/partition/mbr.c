#include "mbr.h"
#include "partition.h"

#include <types.h>
#include <storage/block.h>
#include <memory/alloc.h>
#include <string/utils.h>
#include <video/print.h>

boolean probeMBRPartition(BlockDevice *device) {
    u8 *buf = aalloc(device -> sectorSize, 128);
    if(buf == NULL) return false;

    if(!ReadBlockDevice(device, buf, 0, 1)) {
        free(buf);
        return false;
    }

    boolean isMBR = buf[510] == 0x55 && buf[511] == 0xAA;
    free(buf);

    return isMBR;
}

u32 registerMBRPartitionBlocks(BlockDevice *device) {
    u8 *buf = aalloc(device -> sectorSize, 128);
    if(buf == NULL) return 0;

    ReadBlockDevice(device, buf, 0, 1);
    
    u32 count = 0;
    for(int i = 0; i < 4; i++) {
        u8 *entry = buf + 0x1BE + (i << 4);
        if(entry[4] == 0) continue;

        BlockDevice *dev = malloc(sizeof(BlockDevice));
        if(dev == NULL) continue;

        Partition *part = malloc(sizeof(Partition));
        if(part == NULL) {
            free(dev);
            continue;
        }

        wString model = malloc(sizeof(wchar) * (wcslen(device -> model) + 18 + 1));
        if(model == NULL) {
            free(part);
            free(dev);
            continue;
        }

        slprintf(model, L"%ls - partition.mbr.%d", device -> model, i);

        part -> type = entry[4];
        part -> lbaStart = *(u32 *) (entry + 8);
        part -> parent = device;

        dev -> internal = part;
        dev -> sectorCount = *(u32 *) (entry + 12);
        dev -> model = model;

        ApplyGenericPartitionData(device, dev);

        count++;
        if(!RegisterBlockDevice(dev)) {
            free(model);
            free(part);
            free(dev);
            count--;
        }
    }

    free(buf);
    return count;
}

boolean RegisterMBRPartitionController() {
    PartitionController *ctrl = malloc(sizeof(PartitionController));
    if(ctrl == NULL) return false;

    ctrl -> probe = probeMBRPartition;
    ctrl -> register_blocks = registerMBRPartitionBlocks;

    return RegisterPartitionController(ctrl);
}