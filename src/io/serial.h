#ifndef HH_IO_SERIAL
#define HH_IO_SERIAL

#include <types.h>

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

void InitializeSerial(word port, void (*onReceived)(byte));
int SerialPrintf(const String fmt, ...);
void WriteSerial(byte *data, u16 len);
boolean SerialActive();

#endif