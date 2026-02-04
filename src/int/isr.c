#include "../types.h"
#include "interupt.h"

#include "../file-formats/psf.h"
#include "../graphics/gop.h"

#define ISR_COUNT 48

static void (*handlers[ISR_COUNT])(byte idtIdx) = {0};
static const String exceptions[32] = {
    "Divide by zero",
    "Debug",
    "Non maskable interupt",
    "Breakpoint",
    "Overflow",
    "Out of bounds",
    "Invalid opcode",
    "No coprocessor",
    "Double fault",
    "Coprocessor segment overrun",
    "Bad task state segment",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Unrecognized interrupt",
    "Coprocessor fault",
    "Alignment check",
    "Machine check",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED"
};

void exceptionStub() {
    
}