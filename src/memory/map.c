#include "map.h"
#include <types.h>

// for some reason indexing an array of strings
// will ALWAYS return null, so i have to do this shit
wString MemoryTypeString(u32 type) {
    switch(type) {
        case EFI_ReservedMemoryType      : return L"EfiReservedMemoryType";
        case EFI_LoaderCode              : return L"EfiLoaderCode";
        case EFI_LoaderData              : return L"EfiLoaderData";
        case EFI_BootServicesCode        : return L"EfiBootServicesCode";
        case EFI_BootServicesData        : return L"EfiBootServicesData";
        case EFI_RuntimeServicesCode     : return L"EfiRuntimeServicesCode";
        case EFI_RuntimeServicesData     : return L"EfiRuntimeServicesData";
        case EFI_ConventionalMemory      : return L"EfiConventionalMemory";
        case EFI_UnusableMemory          : return L"EfiUnusableMemory";
        case EFI_ACPIReclaimMemory       : return L"EfiACPIReclaimMemory";
        case EFI_ACPIMemoryNVS           : return L"EfiACPIMemoryNVS";
        case EFI_MemoryMappedIO          : return L"EfiMemoryMappedIO";
        case EFI_MemoryMappedIOPortSpace : return L"EfiMemoryMappedIOPortSpace";
        case EFI_PalCode                 : return L"EfiPalCode";
        case EFI_PersistentMemory        : return L"EfiPersistentMemory";
    }

    return L"Unknown";
}