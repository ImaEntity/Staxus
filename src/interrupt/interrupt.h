#ifndef HH_INTERUPT
#define HH_INTERUPT

#include <types.h>

void RegisterIRQ(byte irqNum, void (*handler)());
void InitializeInterrupts();
void FinalizeInterrupts();

#endif