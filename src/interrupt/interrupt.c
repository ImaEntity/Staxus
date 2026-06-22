#include "interrupt.h"
#include <types.h>

#include "idt.h"
#include "isr.h"
#include "irq.h"

void InitializeInterrupts() {
    initalizeIDT();
    initalizeISRs();
    initalizeIRQs();
}

void FinalizeInterrupts() {
    loadIDT();
}

void RegisterIRQ(byte irqNum, void (*handler)()) {
    installIRQ(irqNum, (void *) handler);
}
