#include <types.h>
#include "keyboard.h"

static u16 scancodeTable[] = {
     0 ,  0 , '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', '-', '=',  0 ,  0 ,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']',  0 ,  0 , 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'','`',  0 , '\\','z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/',  0 , '*',
     0 , ' '
};

u8 inportb(u16 port) {
    u8 ret;

    // i fkn hate this syntax
    asm volatile("inb %1, %0" : "=a"(ret) : "d"(port));
    return ret;
}

u16 GetScancode() {
    if((inportb(0x64) & 1) == 0)
        return KEY_NOT_READY;
    
    return inportb(0x60);
}

u16 TranslateScancode(u16 scancode) {
    return scancodeTable[scancode];
}