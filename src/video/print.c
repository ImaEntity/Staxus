#include "print.h"
#include <types.h>

#include <string/format.h>
#include <string/utils.h>
#include <memory/utils.h>
#include <video/gop.h>
#include <stdarg.h>

u64 HexString(u64 hex, wString buf, boolean useCaps) {
    wchar hexLetterStart = 'a';
    if(useCaps) hexLetterStart = 'A';

    byte i = 0;
    while(hex > 0) {
        byte digit = hex % 16;
        buf[i] = digit < 10 ? digit + '0' : digit - 10 + hexLetterStart;
        
        hex /= 16;
        i++;
    }

    if(i == 0) buf[i++] = '0';
    buf[i] = '\0';

    for(byte j = 0; j < i / 2; j++) {
        byte temp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = temp;
    }

    return i;
}

void vslprintf(wString buf, wString fmt, va_list args) {
    u64 idx = 0;
    while(*fmt != '\0') {
        if(*fmt != '%') {
            buf[idx++] = *fmt;
            fmt++;
            continue;
        }

        fmt++;
        switch(*fmt) {
            case 's': {
                u64 i = 0;
                String str = va_arg(args, String);
                if(str != NULL) while(str[i] != '\0') buf[idx++] = str[i++];
                else buf[idx++] = '(', buf[idx++] = 'n', buf[idx++] = 'u', buf[idx++] = 'l', buf[idx++] = 'l', buf[idx++] = ')';

                break;
            }

            case 'c':
                buf[idx++] = va_arg(args, int);
                break;

            case '%':
                buf[idx++] = '%';
                break;
            
            case 'u': {
                fmt++;
                if(*fmt == 'l') {
                    fmt++;

                    if(*fmt == 'l') idx += UIntToString(va_arg(args, u64), buf + idx, 10);
                    else {
                        idx += UIntToString(va_arg(args, u32), buf + idx, 10);
                        fmt--;
                    }
                } else {
                    idx += UIntToString(va_arg(args, u32), buf + idx, 10);
                    fmt--;
                }

                break;
            }

            case 'd':
                idx += IntToString(va_arg(args, i32), buf + idx, 10);
                break;

            case 'l': {
                fmt++;
                
                if(*fmt == 'l') {
                    u64 pIdx = idx;

                    if(*(fmt + 1) == 'X')      idx += HexString(va_arg(args, u64), buf + idx, true);
                    else if(*(fmt + 1) == 'x') idx += HexString(va_arg(args, u64), buf + idx, false);
                    else if(*(fmt + 1) == 'u') idx += UIntToString(va_arg(args, u64), buf + idx, 10);
                    else if(*(fmt + 1) == 'b') idx += UIntToString(va_arg(args, u64), buf + idx, 2);

                    if(pIdx != idx) fmt++;
                    else idx += IntToString(va_arg(args, i64), buf + idx, 10);
                } else if(*fmt == 'f') idx += FloatToString(va_arg(args, double), buf + idx, 10, 15);
                else if(*fmt == 's') {
                    u64 i = 0;
                    wString str = va_arg(args, wString);

                    if(str != NULL) while(str[i] != '\0') buf[idx++] = str[i++];
                    else buf[idx++] = '(', buf[idx++] = 'n', buf[idx++] = 'u', buf[idx++] = 'l', buf[idx++] = 'l', buf[idx++] = ')';
                } else idx += IntToString(va_arg(args, i32), buf + idx, 10);

                break;
            }

            case '.': {
                fmt++;
                byte precision = *fmt - '0';

                fmt++;
                if(*fmt == 'f') idx += FloatToString(va_arg(args, double), buf + idx, 10, precision);
                else fmt--;

                break;
            }

            case 'f':
                idx += FloatToString(va_arg(args, double), buf + idx, 10, 7);
                break;
            
            case 'p': {
                wchar hex[] = L"0123456789abcdef";
                u64 ptr = (u64) va_arg(args, void *);

                buf[idx++] = '0';
                buf[idx++] = 'x';
                buf[idx++] = hex[(ptr & 0xF000000000000000) >> 60];
                buf[idx++] = hex[(ptr & 0x0F00000000000000) >> 56];
                buf[idx++] = hex[(ptr & 0x00F0000000000000) >> 52];
                buf[idx++] = hex[(ptr & 0x000F000000000000) >> 48];
                buf[idx++] = hex[(ptr & 0x0000F00000000000) >> 44];
                buf[idx++] = hex[(ptr & 0x00000F0000000000) >> 40];
                buf[idx++] = hex[(ptr & 0x000000F000000000) >> 36];
                buf[idx++] = hex[(ptr & 0x0000000F00000000) >> 32];
                buf[idx++] = hex[(ptr & 0x00000000F0000000) >> 28];
                buf[idx++] = hex[(ptr & 0x000000000F000000) >> 24];
                buf[idx++] = hex[(ptr & 0x0000000000F00000) >> 20];
                buf[idx++] = hex[(ptr & 0x00000000000F0000) >> 16];
                buf[idx++] = hex[(ptr & 0x000000000000F000) >> 12];
                buf[idx++] = hex[(ptr & 0x0000000000000F00) >>  8];
                buf[idx++] = hex[(ptr & 0x00000000000000F0) >>  4];
                buf[idx++] = hex[(ptr & 0x000000000000000F) >>  0];

                break;
            }

            case 'x':
                idx += HexString(va_arg(args, u32), buf + idx, false);
                break;
            
            case 'X':
                idx += HexString(va_arg(args, u32), buf + idx, true);
                break;

            case 'b':
                idx += UIntToString(va_arg(args, u32), buf + idx, 2);
                break;
        }

        fmt++;
    }

    buf[idx] = '\0';
}

void slprintf(wString buf, wString fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vslprintf(buf, fmt, args);
    va_end(args);
}

static FrameBuffer *defaultFB;
static PSFFont *defaultFont;
static u16 charX;
static u16 charY;
void InitializePrint(FrameBuffer *fb, PSFFont *font) {
    defaultFB = fb;
    defaultFont = font;
    charX = 0;
    charY = 0;
}

void lprintf(wString fmt, ...) {
    static wchar buf[1024];
    memset(buf, 0, sizeof(buf));
    
    va_list args;
    va_start(args, fmt);
    vslprintf(buf, fmt, args);
    va_end(args);

    if((charY + 1) * defaultFont -> header -> charSize >= defaultFB -> Height) {
        ShiftScreenUp(defaultFB, defaultFont -> header -> charSize);
        charY--;
    }

    for(u64 i = 0; buf[i] != '\0'; i++) {
        if(buf[i] == '\n') {
            charY++;
            charX = 0;
            continue;
        }

        if(buf[i] == '\r') {
            charX = 0;
            continue;
        }

        DrawChar(defaultFB, defaultFont, buf[i], charX * 8, charY * defaultFont -> header -> charSize, 0x999999);
        charX++;
    }
}


void vsprintf(String buf, String fmt, va_list args) {
    static wchar wbuf[1024];
    static wchar wfmt[1024];

    memset(wbuf, 0, sizeof(wbuf));
    memset(wfmt, 0, sizeof(wfmt));
    
    int p = 0;
    while(fmt[p] != 0) {wfmt[p] = fmt[p]; p++;}
    wfmt[p] = 0;

    vslprintf(wbuf, wfmt, args);

    p = 0;
    while(wbuf[p] != 0) {buf[p] = wbuf[p]; p++;}
    buf[p] = 0;
}

void sprintf(String buf, String fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
}

void printf(String fmt, ...) {
    static char buf[1024];
    static wchar wbuf[1024];

    memset(buf, 0, sizeof(buf));
    memset(wbuf, 0, sizeof(wbuf));

    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    int p = 0;
    while(buf[p] != 0) {wbuf[p] = buf[p]; p++;}
    wbuf[p] = 0;

    lprintf(wbuf);
}