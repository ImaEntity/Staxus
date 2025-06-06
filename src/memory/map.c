#include "../types.h"
#include "map.h"

// for some reason indexing an array of strings
// will ALWAYS return null, so i have to do this shit
wString MemoryTypeString(u32 type) {
    switch(type) {
        case  0: return L"EfiReservedMemoryType";
        case  1: return L"EfiLoaderCode";
        case  2: return L"EfiLoaderData";
        case  3: return L"EfiBootServicesCode";
        case  4: return L"EfiBootServicesData";
        case  5: return L"EfiRuntimeServicesCode";
        case  6: return L"EfiRuntimeServicesData";
        case  7: return L"EfiConventionalMemory";
        case  8: return L"EfiUnusableMemory";
        case  9: return L"EfiACPIReclaimMemory";
        case 10: return L"EfiACPIMemoryNVS";
        case 11: return L"EfiMemoryMappedIO";
        case 12: return L"EfiMemoryMappedIOPortSpace";
        case 13: return L"EfiPalCode";
        case 14: return L"EfiPersistentMemory";
    }

    return L"Unknown";
}