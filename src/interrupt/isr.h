#ifndef HH_INTERUPT_ISR
#define HH_INTERUPT_ISR

#include <types.h>

void installISR(byte idtIdx, void (*handler)(byte idtIdx));
void initalizeISRs();

#endif