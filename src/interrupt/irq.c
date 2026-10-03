#include "irq.h"
#include <types.h>

#include "isr.h"
#include "apic.h"

#define PIC_MODE_8086 0x01
#define ICW1_ICW4     0x01
#define ICW1_INIT     0x10
#define PIC1          0x20
#define PIC1_DATA     0x21
#define PIC1_OFFSET   0x20
#define PIC2          0xA0
#define PIC2_DATA     0xA1
#define PIC2_OFFSET   0x28

static inline void outb(u16 port, u8 value) {
    asm volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

static void (*handlers[32])(Registers *r) = {0};
void stub(Registers *r, byte index) {
    if(index <= 47 && index >= 32) {
        if(handlers[index - 32] != NULL)
            handlers[index - 32](r);
    }

    *(volatile dword *) (GetLAPICBase() + APIC_EOI) = 0;
}

void clearMask(byte idx) {
    if(idx > 24) return;
    
    u8 index = 0x10 + 2 * idx;
    u32 low = ReadIOAPICRegister(index);
    
    WriteIOAPICRegister(index, low & ~IOAPIC_INTERRUPT_DISABLED);
}

void installIRQ(byte irqNum, void (*handler)(Registers *r)) {
    // asm volatile("cli");
    handlers[irqNum] = handler;
    clearMask(irqNum);
    // asm volatile("sti");
}

void disablePIC() {
    // this part might not be needed?
    // outb(PIC1, ICW1_INIT | ICW1_ICW4);
    // outb(PIC2, ICW1_INIT | ICW1_ICW4);

    // outb(PIC1_DATA, PIC1_OFFSET);
    // outb(PIC2_DATA, PIC2_OFFSET);

    // outb(PIC1_DATA, 0x04); // PIC2 at IRQ2
    // outb(PIC2_DATA, 0x02); // Cascade indentity

    // outb(PIC1_DATA, PIC_MODE_8086);
    // outb(PIC2_DATA, PIC_MODE_8086);

    // this part is
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

boolean initalizeIRQs() {
    disablePIC();

    if(!DetectLAPIC()) return false;
    EnableLAPIC();

    u8 lapicID = GetLAPICID();
    u8 maxEntries = (ReadIOAPICRegister(0x01) >> 16) & 0xFF;
    for(u8 irq = 0; irq <= maxEntries && irq < 24; irq++)
        SetIOAPICRedirection(irq, irq + 32, lapicID, true);

    for(byte i = 0; i < 16; i++) installISR(i + 32, stub);
    return true;
}