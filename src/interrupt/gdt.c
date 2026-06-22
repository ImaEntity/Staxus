#include "gdt.h"
#include <types.h>

#pragma pack(push, 1)

typedef struct {
    u16 limit_low;
    u16 base_low;
    u8  base_mid;
    u8  access;
    u8  granularity;
    u8  base_high;
} GDTEntry;

typedef struct {
    u16 size;
    u64 offset;
} GDTPointer;

#pragma pack(pop)

static GDTEntry entries[3];
static GDTPointer descriptor;
void InitializeGDT() {
    entries[0] = (GDTEntry) {0};

    entries[1].limit_low = 0xFFFF;
    entries[1].base_low = 0;
    entries[1].base_mid = 0;
    entries[1].access = 0x9A; // present, ring 0, code, RX
    entries[1].granularity = 0xAF; // 64-bit, 4k
    entries[1].base_high = 0;

    entries[2].limit_low = 0xFFFF;
    entries[2].base_low = 0;
    entries[2].base_mid = 0;
    entries[2].access = 0x92; // present, ring 0, data, WR
    entries[2].granularity = 0xAF; // 64-bit, 4k
    entries[2].base_high = 0;

    descriptor.size = sizeof(entries) - 1;
    descriptor.offset = (u64) entries;

    // icl chatgpt wrote ts
    asm volatile(
        "lgdt %0\n"
        "pushq $0x0008\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movw $0x0010, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "movw %%ax, %%ss\n"
        :: "m"(descriptor) : "rax", "memory"
    );
}