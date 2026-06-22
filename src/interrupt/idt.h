#ifndef HH_INTERUPT_IDT
#define HH_INTERUPT_IDT

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
} InteruptTableEntry;

typedef struct {
    word  size;
    qword offset;
} InteruptTablePointer;

#pragma pack(pop)

void setIDTEntry(byte index, void *offset, word sel, byte flags);
void initalizeIDT();
void loadIDT();

#endif
