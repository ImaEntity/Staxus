#ifndef HH_INTERRUPT_ISR
#define HH_INTERRUPT_ISR

#include <types.h>

typedef struct {
    u64 r15; u64 r14; u64 r13; u64 r12;
    u64 r11; u64 r10; u64  r9; u64  r8;
    u64 rbp; u64 rdi; u64 rsi;
    u64 rdx; u64 rcx; u64 rbx; u64 rax;

    u64 errorCode;
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} Registers;

void installISR(byte idtIdx, void (*handler)(Registers *r, byte idtIdx));
void initalizeISRs();

#endif