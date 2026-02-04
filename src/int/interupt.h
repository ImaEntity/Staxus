#ifndef HH_INTERUPTS
#define HH_INTERUPTS

#include <types.h>

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

void InitializeInterupts();
void RegisterIRQ(byte irqNum, void (*handler)());

#endif