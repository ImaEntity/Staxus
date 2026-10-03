#include "interrupt.h"
#include <types.h>

#include "idt.h"
#include "isr.h"
#include "irq.h"

boolean InitializeInterrupts() {
    initalizeIDT();
    initalizeISRs();
    if(!initalizeIRQs()) return false;
    
    loadIDT();
    asm volatile("sti");
    return true;
}

void RegisterIRQ(byte irqNum, void (*handler)(Registers *r)) {
    installIRQ(irqNum, handler);
}
