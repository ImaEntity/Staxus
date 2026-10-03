#include "serial.h"
#include <types.h>

#include <interrupt/interrupt.h>
#include <interrupt/isr.h>
#include <memory/utils.h>
#include <video/print.h>
#include <stdarg.h>

static inline byte inb(word port) {
    byte v;
    asm volatile("inb %1, %0" : "=a"(v) : "d"(port));
    return v;
}

static inline void outb(word port, byte v) {
    asm volatile("outb %0, %1" :: "a"(v), "d"(port));
}

static void (*activeRecv)(byte) = NULL;

word activePort = -1;
inline boolean SerialActive() {
    return activePort != -1;
}

static void serialRecv(Registers *r) {
    if(!SerialActive()) return;

    byte data = inb(activePort);
    if(activeRecv != NULL) activeRecv(data);
}

void InitializeSerial(word port, void (*onReceived)(byte)) {
    outb(port + 1, 1); // enable recv interrupts
    RegisterIRQ(((port & 0xF00) >> 8) + 1, serialRecv);

    activeRecv = onReceived;
    activePort = port;
}

void WriteSerial(byte *data, u16 len) {
    if(!SerialActive()) return;
    for(u16 i = 0; i < len; i++)
        outb(activePort, data[i]);
}

int SerialPrintf(const String fmt, ...) {
    va_list args;
    va_start(args, fmt);

    static char buf[1024];
    memset(buf, 0, sizeof(buf));

    int len = vsprintf(buf, fmt, args);
    va_end(args);

    WriteSerial((byte *) buf, len);
    return len;
}