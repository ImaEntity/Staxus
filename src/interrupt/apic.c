#include "apic.h"
#include <types.h>

inline void WriteIOAPICRegister(u8 reg, u32 val) {
    *(volatile dword *) (IOAPIC_DEFAULT_ADDR + IOAPIC_REGSEL) = reg;
    *(volatile dword *) (IOAPIC_DEFAULT_ADDR + IOAPIC_IOWIN ) = val;
}

inline u32 ReadIOAPICRegister(u8 reg) {
    *(volatile dword *) (IOAPIC_DEFAULT_ADDR + IOAPIC_REGSEL) = reg;
    return *(volatile dword *) (IOAPIC_DEFAULT_ADDR + IOAPIC_IOWIN);
}

boolean DetectLAPIC() {
    int cpuid[4];
    asm volatile("cpuid" : "=a"(cpuid[0]), "=b"(cpuid[1]), "=c"(cpuid[2]), "=d"(cpuid[3]) : "a"(1));
    return (cpuid[3] & (1 << 9)) != 0;
}

static inline void wrmsr(u32 msr, u64 val) {
    u32 lo = val & 0xFFFFFFFF; u32 hi = val >> 32;
    asm volatile("wrmsr" :: "a"(lo), "d"(hi), "c"(msr));
}

static inline u64 rdmsr(u32 msr) {
    u32 lo; u32 hi;
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((u64) hi << 32) | lo;
}

static qword lapicBase = -1;
inline qword GetLAPICBase() {return lapicBase;}

void EnableLAPIC() {
    lapicBase = rdmsr(APIC_BASE_MSR) & 0xFFFFFF000; // bottom 12 bits are flags or smth
    wrmsr(APIC_BASE_MSR, lapicBase | APIC_BASE_MSR_ENABLE);
    volatile dword *svr = (volatile dword *) (lapicBase + APIC_SVR);
    *svr = APIC_SVR_ENABLE | 0x01; // idk fire debug or smth
}

void SetIOAPICRedirection(u8 irq, u8 vector, u8 apicID, boolean masked) {
    u8 index = 0x10 + 2 * irq;
    WriteIOAPICRegister(index, vector | (masked ? IOAPIC_INTERRUPT_DISABLED : 0));
    WriteIOAPICRegister(index + 1, (u32) apicID << 24);
}

u8 GetLAPICID() {
    dword id = *(volatile u32 *) (lapicBase + 0x20);
    return (u8) (id >> 24);
}