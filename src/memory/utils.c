#include "utils.h"
#include <types.h>

void *memset(void *dst, int value, u64 size) {
    for(u64 off = 0; off < size; off++)
        *((byte *) dst + off) = (byte) value;
        
    return dst;
}

void *memcpy(void *dst, const void *src, u64 size) {
    for(u64 off = 0; off < size; off++)
        *((byte *) dst + off) = *((byte *) src + off);
        
    return dst;
}

int memcmp(const void *blk1, const void *blk2, u64 size) {
    for(u64 off = 0; off < size; off++) {
        int diff = *((byte *) blk1 + off) - *((byte *) blk2 + off);
        if(diff != 0) return diff;
    }

    return 0;
}

void *memchr(const void *src, int value, u64 size) {
    for(u64 off = 0; off < size; off++)
        if(*((byte *) src + off) == (byte) value) return (byte *) src + off;

    return NULL;
}
