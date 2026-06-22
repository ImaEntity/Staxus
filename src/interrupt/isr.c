#include <types.h>
#include "interrupt.h"

#include "idt.h"
#include <video/gop.h>
#include <string/utils.h>

#define ISR_COUNT 48

static void (*handlers[ISR_COUNT])(byte idtIdx) = {0};
void handleISR() {
    register byte idtIdx asm("rdi");
    if(handlers[idtIdx] != NULL) handlers[idtIdx](idtIdx);
}

#define ISR(n) void isr_##n(byte idtIdx); asm(".intel_syntax noprefix\n" \
    ".global isr_" #n "       \n"                                        \
    "isr_" #n ":              \n"                                        \
    "    cli                  \n"                                        \
    "                         \n"                                        \
    "    push rax             \n"                                        \
    "    push rcx             \n"                                        \
    "    push rdx             \n"                                        \
    "    push rbx             \n"                                        \
    "    push rbp             \n"                                        \
    "    push rsi             \n"                                        \
    "    push rdi             \n"                                        \
    "    push r8              \n"                                        \
    "    push r9              \n"                                        \
    "    push r10             \n"                                        \
    "    push r11             \n"                                        \
    "    push r12             \n"                                        \
    "    push r13             \n"                                        \
    "    push r14             \n"                                        \
    "    push r15             \n"                                        \
    "                         \n"                                        \
    "    mov rdi, " #n "      \n"                                        \
    "    sub rsp, 8           \n"                                        \
    "    call handleISR       \n"                                        \
    "    add rsp, 8           \n"                                        \
    "                         \n"                                        \
    "    pop r15              \n"                                        \
    "    pop r14              \n"                                        \
    "    pop r13              \n"                                        \
    "    pop r12              \n"                                        \
    "    pop r11              \n"                                        \
    "    pop r10              \n"                                        \
    "    pop  r9              \n"                                        \
    "    pop  r8              \n"                                        \
    "    pop rdi              \n"                                        \
    "    pop rsi              \n"                                        \
    "    pop rbp              \n"                                        \
    "    pop rbx              \n"                                        \
    "    pop rdx              \n"                                        \
    "    pop rcx              \n"                                        \
    "    pop rax              \n"                                        \
    "                         \n"                                        \
    "    iretq                \n"                                        \
    ".att_syntax noprefix\n");

ISR(0)  ISR(1)  ISR(2)  ISR(3)
ISR(4)  ISR(5)  ISR(6)  ISR(7)
ISR(8)  ISR(9)  ISR(10) ISR(11)
ISR(12) ISR(13) ISR(14) ISR(15)
ISR(16) ISR(17) ISR(18) ISR(19)
ISR(20) ISR(21) ISR(22) ISR(23)
ISR(24) ISR(25) ISR(26) ISR(27)
ISR(28) ISR(29) ISR(30) ISR(31)

// IRQs
ISR(32) ISR(33) ISR(34) ISR(35)
ISR(36) ISR(37) ISR(38) ISR(39)
ISR(40) ISR(41) ISR(42) ISR(43)
ISR(44) ISR(45) ISR(46) ISR(47)

static void (*stubs[ISR_COUNT])(byte idtIdx) = {
    isr_0 , isr_1 , isr_2 , isr_3 , isr_4 , isr_5 , isr_6 , isr_7 ,
    isr_8 , isr_9 , isr_10, isr_11, isr_12, isr_13, isr_14, isr_15,
    isr_16, isr_17, isr_18, isr_19, isr_20, isr_21, isr_22, isr_23,
    isr_24, isr_25, isr_26, isr_27, isr_28, isr_29, isr_30, isr_31,
    isr_32, isr_33, isr_34, isr_35, isr_36, isr_37, isr_38, isr_39,
    isr_40, isr_41, isr_42, isr_43, isr_44, isr_45, isr_46, isr_47
};

static const wString exceptions[32] = {
    L"Divide by zero",
    L"Debug",
    L"Non maskable interupt",
    L"Breakpoint",
    L"Overflow",
    L"Out of bounds",
    L"Invalid opcode",
    L"No coprocessor",
    L"Double fault",
    L"Coprocessor segment overrun",
    L"Bad task state segment",
    L"Segment not present",
    L"Stack fault",
    L"General protection fault",
    L"Page fault",
    L"Unrecognized interrupt",
    L"Coprocessor fault",
    L"Alignment check",
    L"Machine check",
    L"RESERVED EXCEPTION",
    L"RESERVED EXCEPTION",
    L"RESERVED EXCEPTION",
    L"RESERVED EXCEPTION",
    L"RESERVED EXCEPTION",
    L"RESERVED EXCEPTION",
    L"RESERVED EXCEPTION",
    L"RESERVED EXCEPTION",
    L"RESERVED EXCEPTION",
    L"RESERVED EXCEPTION",
    L"RESERVED EXCEPTION",
    L"RESERVED EXCEPTION",
    L"RESERVED EXCEPTION"
};

FrameBuffer *getFrameBuffer();
PSFFont *getDefaultFont();

void exceptionStub(byte idtIdx) {
    wString errMessage = exceptions[idtIdx];
    FrameBuffer *frame = getFrameBuffer();
    PSFFont *font = getDefaultFont();
    
    DrawString(
        frame, font,
        errMessage,
        frame -> Width / 2 - (wcslen(errMessage) * 8) / 2,
        frame -> Height / 2 - font -> header -> charSize / 2,
        0xFF0000
    );
}

void installISR(byte idtIdx, void (*handler)(byte idtIdx)) {
    handlers[idtIdx] = handler;
}

void initalizeISRs() {
    for(byte i = 0; i < ISR_COUNT; i++)
        setIDTEntry(i, stubs[i], 0x0008, 0x8E);

    setIDTEntry(1, stubs[1], 0x0008, 0x8F);
    setIDTEntry(3, stubs[3], 0x0008, 0x8F);
    setIDTEntry(4, stubs[4], 0x0008, 0x8F);

    for(byte i = 0; i < 32; i++)
        installISR(i, exceptionStub);
}