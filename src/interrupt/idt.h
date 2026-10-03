#ifndef HH_INTERRUPT_IDT
#define HH_INTERRUPT_IDT

#include <types.h>
#include "interrupt.h"

#pragma pack(push, 1)

typedef struct {
    word  lOff;
    word  sel;
    byte  ist;
    byte  flags;
    word  mOff;
    dword hOff;
    dword zero;
} InterruptTableEntry;

typedef struct {
    word  size;
    qword offset;
} InterruptTablePointer;

#pragma pack(pop)

void setIDTEntry(byte index, void (*offset)(byte), word sel, byte flags);
void initalizeIDT();
void loadIDT();

#endif
