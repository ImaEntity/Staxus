#ifndef HH_INTERRUPT
#define HH_INTERRUPT

#include <types.h>
#include "isr.h"

void RegisterIRQ(byte irqNum, void (*handler)(Registers *r));
boolean InitializeInterrupts();

#endif