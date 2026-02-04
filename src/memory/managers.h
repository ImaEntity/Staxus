#ifndef HH_MEM_MANAGERS
#define HH_MEM_MANAGERS

#include <memory/alloc.h>

MemoryManager mmanager_get_identity();
#define IDENTITY_ALLOCATOR mmanager_get_identity()

#endif