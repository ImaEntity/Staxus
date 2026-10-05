#include "isr.h"
#include <types.h>

#include "idt.h"
#include "interrupt.h"
#include <io/serial.h>
#include <video/gop.h>
#include <video/print.h>
#include <string/utils.h>

#define ISR_COUNT 48

static void (*handlers[ISR_COUNT])(Registers *r, byte idtIdx) = {0};
void handleISR(Registers *r, byte idtIdx) {
    if(handlers[idtIdx] != NULL)
        handlers[idtIdx](r, idtIdx);
}

#define ISR(n)                     \
    void isr_##n(byte idtIdx);     \
    asm(".intel_syntax noprefix\n" \
    ".global isr_" #n "        \n" \
    "isr_" #n ":               \n" \
    "    push 0                \n" \
    "    push rax              \n" \
    "    push rbx              \n" \
    "    push rcx              \n" \
    "    push rdx              \n" \
    "    push rsi              \n" \
    "    push rdi              \n" \
    "    push rbp              \n" \
    "    push r8               \n" \
    "    push r9               \n" \
    "    push r10              \n" \
    "    push r11              \n" \
    "    push r12              \n" \
    "    push r13              \n" \
    "    push r14              \n" \
    "    push r15              \n" \
    "                          \n" \
    "    mov rbp, rsp          \n" \
    "    and rsp, -16          \n" \
    "    sub rsp, 32           \n" \
    "                          \n" \
    "    mov rcx, rbp          \n" \
    "    mov rdx, " #n "       \n" \
    "    call handleISR        \n" \
    "                          \n" \
    "    mov rsp, rbp          \n" \
    "                          \n" \
    "    pop r15               \n" \
    "    pop r14               \n" \
    "    pop r13               \n" \
    "    pop r12               \n" \
    "    pop r11               \n" \
    "    pop r10               \n" \
    "    pop r9                \n" \
    "    pop r8                \n" \
    "    pop rbp               \n" \
    "    pop rdi               \n" \
    "    pop rsi               \n" \
    "    pop rdx               \n" \
    "    pop rcx               \n" \
    "    pop rbx               \n" \
    "    pop rax               \n" \
    "    add rsp, 8            \n" \
    "    iretq                 \n" \
    ".att_syntax noprefix");

#define ISR_ERR(n)                 \
    void isr_##n(byte idtIdx);     \
    asm(".intel_syntax noprefix\n" \
    ".global isr_" #n "        \n" \
    "isr_" #n ":               \n" \
    "    push rax              \n" \
    "    push rbx              \n" \
    "    push rcx              \n" \
    "    push rdx              \n" \
    "    push rsi              \n" \
    "    push rdi              \n" \
    "    push rbp              \n" \
    "    push r8               \n" \
    "    push r9               \n" \
    "    push r10              \n" \
    "    push r11              \n" \
    "    push r12              \n" \
    "    push r13              \n" \
    "    push r14              \n" \
    "    push r15              \n" \
    "                          \n" \
    "    mov rbp, rsp          \n" \
    "    and rsp, -16          \n" \
    "    sub rsp, 32           \n" \
    "                          \n" \
    "    mov rcx, rbp          \n" \
    "    mov rdx, " #n "       \n" \
    "    call handleISR        \n" \
    "                          \n" \
    "    mov rsp, rbp          \n" \
    "                          \n" \
    "    pop r15               \n" \
    "    pop r14               \n" \
    "    pop r13               \n" \
    "    pop r12               \n" \
    "    pop r11               \n" \
    "    pop r10               \n" \
    "    pop r9                \n" \
    "    pop r8                \n" \
    "    pop rbp               \n" \
    "    pop rdi               \n" \
    "    pop rsi               \n" \
    "    pop rdx               \n" \
    "    pop rcx               \n" \
    "    pop rbx               \n" \
    "    pop rax               \n" \
    "    add rsp, 8            \n" \
    "    iretq                 \n" \
    ".att_syntax noprefix");

ISR(0)      ISR(1)      ISR(2)      ISR(3)
ISR(4)      ISR(5)      ISR(6)      ISR(7)
ISR_ERR(8)  ISR(9)      ISR_ERR(10) ISR_ERR(11)
ISR_ERR(12) ISR_ERR(13) ISR_ERR(14) ISR(15)
ISR(16)     ISR_ERR(17) ISR(18)     ISR(19)
ISR(20)     ISR(21)     ISR(22)     ISR(23)
ISR(24)     ISR(25)     ISR(26)     ISR(27)
ISR(28)     ISR(29)     ISR(30)     ISR(31)

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
    L"Non maskable interrupt",
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

void exceptionStub(Registers *r, byte idtIdx) {
    wString errMessage = exceptions[idtIdx];
    FrameBuffer *frame = getFrameBuffer();

    PSFFont *font = LoadFont("/resources/fonts/D8x16-ext.psf");
    if(font == NULL) font = getDefaultFont();

    DrawString(
        frame, font,
        errMessage,
        frame -> Width / 2 - (wcslen(errMessage) * 8) / 2,
        frame -> Height / 2 - font -> header -> charSize / 2,
        0xFF0000
    );

    if(SerialActive()) {
        SerialPrintf("\n=== EXCEPTION OCCURRED - %ls ===\n", errMessage);
        SerialPrintf("RAX = %p  RBX = %p  RCX = %p  RDX = %p\n", r -> rax, r -> rbx, r -> rcx, r -> rdx);
        SerialPrintf("RSI = %p  RDI = %p  RBP = %p  RSP = %p\n", r -> rsi, r -> rdi, r -> rbp, r -> rsp);
        SerialPrintf("R8  = %p  R9  = %p  R10 = %p  R11 = %p\n", r -> r8,  r -> r9,  r -> r10, r -> r11);
        SerialPrintf("R12 = %p  R13 = %p  R14 = %p  R15 = %p\n", r -> r12, r -> r13, r -> r14, r -> r15);
        SerialPrintf("RIP = %p  CS  = %p  SS  = %p\n",           r -> rip, r -> cs, r -> ss);
        SerialPrintf("ERR = %X  FLG = %X\n",                     r -> errorCode, r -> rflags);
    } else {
        printf("\n=== EXCEPTION OCCURRED - %ls ===\n", errMessage);
        printf("RAX = %p  RBX = %p  RCX = %p  RDX = %p\n", r -> rax, r -> rbx, r -> rcx, r -> rdx);
        printf("RSI = %p  RDI = %p  RBP = %p  RSP = %p\n", r -> rsi, r -> rdi, r -> rbp, r -> rsp);
        printf("R8  = %p  R9  = %p  R10 = %p  R11 = %p\n", r -> r8,  r -> r9,  r -> r10, r -> r11);
        printf("R12 = %p  R13 = %p  R14 = %p  R15 = %p\n", r -> r12, r -> r13, r -> r14, r -> r15);
        printf("RIP = %p  CS  = %p  SS  = %p\n",           r -> rip, r -> cs, r -> ss);
        printf("ERR = %X  FLG = %X\n",                     r -> errorCode, r -> rflags);
    }

    while(1);
}

void installISR(byte idtIdx, void (*handler)(Registers *r, byte idtIdx)) {
    handlers[idtIdx] = handler;
}

void initalizeISRs() {
    for(byte i = 0; i < ISR_COUNT; i++)
        setIDTEntry(i, stubs[i], 0x0008, 0x8E);

    // setIDTEntry(1, stubs[1], 0x0008, 0x8F);
    // setIDTEntry(3, stubs[3], 0x0008, 0x8F);
    // setIDTEntry(4, stubs[4], 0x0008, 0x8F);

    for(byte i = 0; i < 32; i++)
        installISR(i, exceptionStub);
}