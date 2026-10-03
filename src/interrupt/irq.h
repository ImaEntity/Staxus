#ifndef HH_INTERRUPT_IRQ
#define HH_INTERRUPT_IRQ

#include <types.h>

boolean initalizeIRQs();
void installIRQ(byte irqNum, void (*handler)());

#endif