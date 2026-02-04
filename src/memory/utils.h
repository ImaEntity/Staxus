#ifndef HH_MEM_UTILS
#define HH_MEM_UTILS

#include <types.h>

void *memset(void *dst, int value, u64 size);
void *memcpy(void *dst, const void *src, u64 size);
int   memcmp(const void *blk1, const void *blk2, u64 size);
void *memchr(const void *src, int value, u64 size);

#endif