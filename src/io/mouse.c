#include "mouse.h"
#include <types.h>

#include <interrupt/interrupt.h>
#include <interrupt/isr.h>

#define PS2_STATUS 0x64
#define PS2_DATA   0x60

static inline byte inb(word port) {
    byte v;
    asm volatile("inb %1, %0" : "=a"(v) : "d"(port));
    return v;
}

static inline void outb(word port, byte v) {
    asm volatile("outb %0, %1" :: "a"(v), "d"(port));
}

static inline void ps2cmd(byte cmd) {
    // wait for input clear
    while((inb(PS2_STATUS) & 0x02) != 0);
    outb(PS2_STATUS, cmd);
}

static inline void ps2write(byte data) {
    // wait for input clear
    while((inb(PS2_STATUS) & 0x02) != 0);
    outb(PS2_DATA, data);
}

static inline byte ps2read() {
    // wait for output to have data
    while((inb(PS2_STATUS) & 0x01) == 0);
    return inb(PS2_DATA);
}

static inline byte rmouse() {
    while(true) {
        byte status = inb(PS2_STATUS);
        if((status & 0x01) == 0) continue;

        byte data = inb(PS2_DATA);
        if((status & 0x20) != 0) return data;
    }
}

static inline void wmouse(byte data) {
    ps2cmd(0xD4); // next byte to mouse
    ps2write(data);

    while(rmouse() != 0xFA); // ACK
}

static inline void setSampleRate(u8 rate) {
    wmouse(0xF3);
    wmouse(rate);
}

static u8 mouseID = -1;
static void (*onMouseEvent)(MouseEvent) = NULL;

static byte packet[4];
static u8 packetPtr = 0;

static u32 mouseX = 0;
static u32 mouseY = 0;
static byte mouseButtons = 0;

#include <io/serial.h>
static void onMousePacket(Registers *r) {
    byte status = inb(PS2_STATUS);
    while((status & 0x01) != 0) {
        byte data = inb(PS2_DATA);
        if((status & 0x20) != 0)
            packet[packetPtr++] = data;

        status = inb(PS2_STATUS);
    }
    
    // char hex[] = "0123456789ABCDEF \n";
    // for(int i = 0; i < packetPtr; i++) {
    //     byte b = packet[i];
    //     byte data[3];

    //     data[0] = (byte) hex[b >> 4];
    //     data[1] = (byte) hex[b & 15];
    //     data[2] = (byte) hex[16];

    //     WriteSerial(data, 3);
    // }

    // WriteSerial((byte *) hex + 17, 1);

    if(packetPtr < 3) return;
    if(mouseID >= 3 && packetPtr < 4) return;
    packetPtr = 0;

    MouseEvent evt;
    evt.prevX = mouseX; evt.prevY = mouseY;
    evt.prevButtons = mouseButtons;

    i16 deltaX = packet[1] - ((packet[0] << 4) & 0x100);
    i16 deltaY = packet[2] - ((packet[0] << 3) & 0x100);

    mouseX += deltaX; mouseY += deltaY;
    evt.curX = mouseX; evt.curY = mouseY;

    mouseButtons &= ~0x07; // clear LRM button bits
    mouseButtons |= packet[0] & 0x07;

    if(mouseID >= 3) {
        evt.scrollAmount = packet[3] & 0x0F;
        if((packet[3] & 0x08) != 0) evt.scrollAmount |= ~0x0F;
    }

    if(mouseID >= 4) {
        mouseButtons &= ~0x30; // clear X1 and X2 bits
        mouseButtons |= packet[3] & 0x30;
    }

    evt.buttons = mouseButtons;

    if(onMouseEvent != NULL)
        onMouseEvent(evt);
}

u8 InitializeMouse(void (*onEvent)(MouseEvent)) {
    ps2cmd(0xA8); // enable aux port

    ps2cmd(0x20); // get controller status
    byte status = ps2read();

    status |= 0x02; // enable IRQ12
    status &= ~0x20; // enable mouse clock

    ps2cmd(0x60); // set controller status
    ps2write(status);

    wmouse(0xF5); // disable mouse streaming

    // attempt to enable scroll wheel
    setSampleRate(200);
    setSampleRate(100);
    setSampleRate(80);

    wmouse(0xF2); // get mouse id
    mouseID = rmouse();

    if(mouseID == 3) {
        // attempt to enable x buttons
        setSampleRate(200);
        setSampleRate(200);
        setSampleRate(80);

        wmouse(0xF2); // get mouse id
        mouseID = rmouse();
    }

    wmouse(0xE6); // normal scaling

    // 4 counts / mm 
    wmouse(0xE8);
    wmouse(0x03);

    // 200 packets per second
    setSampleRate(200);
    wmouse(0xF4); // enable streaming

    onMouseEvent = onEvent;
    RegisterIRQ(12, onMousePacket);

    return mouseID;
}