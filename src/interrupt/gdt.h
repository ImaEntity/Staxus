#ifndef HH_INTERRUPT_GDT
#define HH_INTERRUPT_GDT

#include <types.h>

void InitializeGDT(u64 r0StackTop);

#endif