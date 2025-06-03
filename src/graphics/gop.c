#include "gop.h"

#include "../types.h"
#include "../file-formats/psf.h"

void ClearScreen(FrameBuffer *fb, u32 color) {
    for(u32 y = 0; y < fb -> Height; y++) {
        for(u32 x = 0; x < fb -> Width; x++) {
            u64 idx = y * fb -> PixelsPerScan + x;
            *((dword *) fb -> BaseAddr + idx) = color;
        }
    }
}

void SetPixel(FrameBuffer *fb, u32 x, u32 y, u32 color) {
    if(x >= fb -> Width || y >= fb -> Height) return;

    u64 idx = y * fb -> PixelsPerScan + x;
    *((dword *) fb -> BaseAddr + idx) = color;
}

u32 GetPixel(FrameBuffer *fb, u32 x, u32 y) {
    if(x >= fb -> Width || y >= fb -> Height) return 0;

    u64 idx = y * fb -> PixelsPerScan + x;
    return *((dword *) fb -> BaseAddr + idx);
}

void DrawLine(FrameBuffer *fb, u32 x0, u32 y0, u32 x1, u32 y1, u32 color) {
    int dx = x1 - x0 < 0 ? -(x1 - x0) : x1 - x0;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 - y0 > 0 ? -(y1 - y0) : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while(true) {
        SetPixel(fb, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;

        e2 = 2 * err;

        if(e2 >= dy) { err += dy; x0 += sx; }
        if(e2 <= dx) { err += dx; y0 += sy; }
    }
}

void DrawChar(FrameBuffer *fb, PSFFont *font, wchar chr, u32 x, u32 y, u32 color) {
    word glyphIdx = font -> unicodeTable[chr];
    byte *glyph = font->glyphBuffer + glyphIdx * font->header->charSize;

    for(u32 i = 0; i < font -> header -> charSize; i++) { // this assumes 1 row = 1 byte
        for(u32 j = 0; j < 8; j++) {
            if(glyph[i] >> (7 - j) & 1 != 0)
                SetPixel(fb, x + j, y + i, color);
        }
    }
}

void DrawString(FrameBuffer *fb, PSFFont *font, wString str, u32 x, u32 y, u32 color) {
    for(u32 i = 0; str[i] != '\0'; i++) {
        DrawChar(fb, font, str[i], x, y, color);
        x += 8;
    }
}