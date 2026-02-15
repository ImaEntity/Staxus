#include "gpt.h"
#include "partition.h"

#include <types.h>
#include <storage/block.h>
#include <memory/utils.h>
#include <memory/alloc.h>
#include <string/utils.h>
#include <video/print.h>

#pragma pack(push, 1)
typedef struct {
    char  signature[8];
    dword revision;
    dword headerSize;
    dword headerCRC32;
    dword reserved;
    qword currentLBA;
    qword backupLBA;
    qword firstUsableLBA;
    qword lastUsableLBA;
    byte  diskGUID[16];
    qword partEntryLBA;
    dword numPartEntries;
    dword sizeOfPartEntry;
    dword partArrayCRC32;
} GPTPartitionHeader;

typedef struct {
    byte  typeGUID[16];
    byte  uniqueGUID[16];
    qword firstLBA;
    qword lastLBA;
    qword attributes;
    wchar name[36];
} GPTPartitionEntry;
#pragma pack(pop)

boolean probeGPTPartition(BlockDevice *device) {
    u8 *buf = aalloc(device -> sectorSize, 128);
    if(buf == NULL) return false;

    if(!ReadBlockDevice(device, buf, 1, 1)) {
        free(buf);
        return false;
    }

    boolean isGPT = memcmp(buf, "EFI PART", 8) == 0;
    free(buf);

    return isGPT;
}

u32 registerGPTPartitionBlocks(BlockDevice *device) {
    u8 *buf = aalloc(device -> sectorSize, 128);
    if(buf == NULL) return 0;

    ReadBlockDevice(device, buf, 1, 1);

    GPTPartitionHeader *header = (GPTPartitionHeader *) buf;
    u32 count = 0;

    u32 entriesPerSector = device -> sectorSize / header -> sizeOfPartEntry;
    for(u32 i = 0; i < header -> numPartEntries; i++) {
        u64 lba = header -> partEntryLBA + i / entriesPerSector;
        u32 offset = (i % entriesPerSector) * header -> sizeOfPartEntry;

        u8 *sector = aalloc(device -> sectorSize, 128);
        if(sector == NULL) continue;

        ReadBlockDevice(device, sector, lba, 1);

        GPTPartitionEntry *entry = (GPTPartitionEntry *) (sector + offset);
        boolean empty = true;
        for(int j = 0; j < 16; j++) {
            if(entry -> typeGUID[j] != 0) {
                empty = false;
                break;
            }
        }

        if(empty) continue;

        BlockDevice *dev = malloc(sizeof(BlockDevice));
        if(dev == NULL) {
            free(sector);
            continue;
        }

        Partition *part = malloc(sizeof(Partition));
        if(part == NULL) {
            free(dev);
            free(sector);
            continue;
        }

        wchar nBuf[16];
        slprintf(nBuf, L"%d", i);

        wString model = malloc(sizeof(wchar) * (wcslen(device -> model) + 17 + wcslen(nBuf) + 1));
        if(model == NULL) {
            free(part);
            free(dev);
            free(sector);
            continue;
        }

        slprintf(model, L"%ls - partition.gpt.%ls", device -> model, nBuf);

        part -> type = 0; // TODO: Map GUID to type enum
        part -> lbaStart = entry -> firstLBA;
        part -> parent = device;

        dev -> internal = part;
        dev -> sectorCount = entry -> lastLBA - entry -> firstLBA + 1;
        dev -> model = model;

        ApplyGenericPartitionData(device, dev);

        count++;
        if(!RegisterBlockDevice(dev)) {
            free(model);
            free(part);
            free(dev);
            free(sector);
            count--;
        }
    }

    return count;
}

boolean RegisterGPTPartitionController() {
    PartitionController *ctrl = malloc(sizeof(PartitionController));
    if(ctrl == NULL) return false;

    ctrl -> probe = probeGPTPartition;
    ctrl -> register_blocks = registerGPTPartitionBlocks;

    return RegisterPartitionController(ctrl);
}