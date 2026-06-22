#ifndef HH_INTERUPT_IRQ
#define HH_INTERUPT_IRQ

#include <types.h>

void initalizeIRQs();
void installIRQ(byte irqNum, void (*handler)(byte index));

#endif