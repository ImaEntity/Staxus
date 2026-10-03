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

typedef struct {
    u32 reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 reserved2;
    u16 reserved3;
    u16 iomap_base;
} TSS;

#pragma pack(pop)

static GDTEntry entries[7] = {0};
static GDTPointer descriptor;
static TSS tss = {0};

void InitializeGDT(u64 r0StackTop) {
    tss.iomap_base = sizeof(TSS);
    tss.rsp0 = r0StackTop;

    // kernel code
    entries[1].limit_low = 0xFFFF;
    entries[1].access = 0x9A; // present, ring 0, code, RX
    entries[1].granularity = 0xAF; // 64-bit, 4k

    // kernel data
    entries[2].limit_low = 0xFFFF;
    entries[2].access = 0x92; // present, ring 0, data, RW
    entries[2].granularity = 0xAF; // 64-bit, 4k

    // user code
    entries[3].limit_low = 0xFFFF;
    entries[3].access = 0xFA; // present, ring 3, code, RX
    entries[3].granularity = 0xAF; // 64-bit, 4k

    // user data
    entries[4].limit_low = 0xFFFF;
    entries[4].access = 0xF2; // present, ring 3, data, RW
    entries[4].granularity = 0xAF; // 64-bit, 4k

    // TSS entrie(s)
    u64 base = (u64) &tss;
    u32 limit = sizeof(TSS) - 1;

    entries[5].limit_low = limit & 0xFFFF;
    entries[5].base_low = base & 0xFFFF;
    entries[5].base_mid = (base >> 16) & 0xFF;
    entries[5].access = 0x89; // present, ring 0, TSS
    entries[5].granularity = (limit >> 16) & 0x0F;
    entries[5].base_high = (base >> 24) & 0xFF;

    entries[6].limit_low = (base >> 32) & 0xFFFF;
    entries[6].base_low = (base >> 48) & 0xFFFF;
    
    descriptor.size = sizeof(entries) - 1;
    descriptor.offset = (u64) entries;

    // icl chatgpt wrote ts
    asm volatile(
        "lgdt %0\n"

        // reload code seg
        "pushq $0x08\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"

        // reload data seg
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "movw %%ax, %%ss\n"

        // load tss
        "movw $0x28, %%ax\n"
        "ltr %%ax"
        :: "m"(descriptor) : "rax", "memory"
    );
}