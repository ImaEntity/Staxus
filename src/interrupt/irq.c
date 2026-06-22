#include "irq.h"
#include <types.h>

#include "isr.h"

#define PIC1        0x20
#define PIC1_DATA   0x21
#define PIC1_OFFSET 0x20

#define PIC2        0xA0
#define PIC2_DATA   0xA1
#define PIC2_OFFSET 0x28

#define PIC_EOI       0x20
#define PIC_MODE_8086 0x01
#define ICW1_ICW4     0x01
#define ICW1_INIT     0x10

#define iowait() (outb(0x80, 0))

static inline void outb(u16 port, u8 value) {
    asm volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void (*handlers[32])(byte index) = {0};
void stub(byte index) {
    if(index <= 47 && index >= 32) {
        if(handlers[index - 32] != NULL)
            handlers[index - 32](index - 32);
    }

    if(index >= PIC2_OFFSET)
        outb(PIC2, PIC_EOI);

    outb(PIC1, PIC_EOI);
}

void clearMask(byte idx) {
    word port = idx < 8 ? PIC1_DATA : PIC2_DATA;

    if(idx >= 8) idx -= 8;
    byte value = inb(port) & ~(1 << idx);

    outb(port, value);
}

void remapIRQ() {
    // byte mask1 = inb(PIC1_DATA);
    // byte mask2 = inb(PIC2_DATA);

    outb(PIC1, ICW1_INIT | ICW1_ICW4); iowait();
    outb(PIC2, ICW1_INIT | ICW1_ICW4); iowait();

    outb(PIC1_DATA, PIC1_OFFSET); iowait();
    outb(PIC2_DATA, PIC2_OFFSET); iowait();

    outb(PIC1_DATA, 0x04); iowait(); // PIC2 at IRQ2
    outb(PIC2_DATA, 0x02); iowait(); // Cascade indentity

    outb(PIC1_DATA, PIC_MODE_8086); iowait();
    outb(PIC2_DATA, PIC_MODE_8086); iowait();

    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void installIRQ(byte irqNum, void (*handler)(byte index)) {
    handlers[irqNum] = handler;
    clearMask(irqNum);
}

void initalizeIRQs() {
    remapIRQ();

    for(byte i = 0; i < 16; i++)
        installISR(i + 32, stub);
}