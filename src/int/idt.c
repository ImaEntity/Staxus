#include "interupt.h"
#include <types.h>

#include <memory/utils.h>

static InteruptTablePointer descriptor;
static InteruptTableEntry entries[256];

void buildIDT() {
    descriptor.size = sizeof(entries) - 1;
    descriptor.offset = (qword) entries;

    memset(entries, 0, sizeof(entries));

    asm volatile("lidt %0" :: "m"(descriptor) : "memory");
}

void setIDTEntry(byte index, void *offset, word sel, byte flags) {
    qword addr = (qword) offset;

    entries[index].lOff =  addr        & 0xFFFF;
    entries[index].mOff = (addr >> 16) & 0xFFFF;
    entries[index].hOff = (addr >> 32);

    entries[index].sel = sel;
    entries[index].zero = 0;
    entries[index].flags = flags;
}