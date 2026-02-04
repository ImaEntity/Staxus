#ifndef HH_MEM_MAP
#define HH_MEM_MAP

#include <types.h>

#define EFI_ReservedMemoryType       0
#define EFI_LoaderCode               1
#define EFI_LoaderData               2
#define EFI_BootServicesCode         3
#define EFI_BootServicesData         4
#define EFI_RuntimeServicesCode      5
#define EFI_RuntimeServicesData      6
#define EFI_ConventionalMemory       7
#define EFI_UnusableMemory           8
#define EFI_ACPIReclaimMemory        9
#define EFI_ACPIMemoryNVS           10
#define EFI_MemoryMappedIO          11
#define EFI_MemoryMappedIOPortSpace 12
#define EFI_PalCode                 13
#define EFI_PersistentMemory        14

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