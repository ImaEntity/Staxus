#ifndef HH_MEMMAP
#define HH_MEMMAP

#include "../types.h"

#define EfiReservedMemoryType       0
#define EfiLoaderCode               1
#define EfiLoaderData               2
#define EfiBootServicesCode         3
#define EfiBootServicesData         4
#define EfiRuntimeServicesCode      5
#define EfiRuntimeServicesData      6
#define EfiConventionalMemory       7
#define EfiUnusableMemory           8
#define EfiACPIReclaimMemory        9
#define EfiACPIMemoryNVS           10
#define EfiMemoryMappedIO          11
#define EfiMemoryMappedIOPortSpace 12
#define EfiPalCode                 13
#define EfiPersistentMemory        14

typedef struct {
    u32 type;
    u32 reserved; // padding
    u64 physAddr;
    u64 virtAddr;
    u64 numPages;
    u64 flags;
} MemoryDescriptor;

typedef struct {
    MemoryDescriptor *map;
    u64 descriptorSize;
    u64 mapSize;
} MemoryMap;

wString MemoryTypeString(u32 type);

#endif