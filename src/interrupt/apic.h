#ifndef HH_INTERRUPT_APIC
#define HH_INTERRUPT_APIC

#include <types.h>

#define IOAPIC_DEFAULT_ADDR       0xFEC00000
#define APIC_BASE_MSR             0x1F
#define APIC_BASE_MSR_ENABLE      0x800
#define APIC_EOI                  0xB0
#define APIC_SVR                  0xF0
#define APIC_SVR_ENABLE           0x100
#define IOAPIC_REGSEL             0x00
#define IOAPIC_IOWIN              0x10
#define IOAPIC_INTERRUPT_DISABLED 0x10000

boolean DetectLAPIC();
qword GetLAPICBase();
void EnableLAPIC();

u8 GetLAPICID();

u32 ReadIOAPICRegister(u8 reg);
void WriteIOAPICRegister(u8 reg, u32 val);

void SetIOAPICRedirection(u8 irq, u8 vector, u8 apicID, boolean masked);

#endif