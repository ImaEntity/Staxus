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


void DrawChar(FrameBuffer *fb, PSFFont *font, byte chr, u32 x, u32 y, u32 color) {
    byte *glyph = font -> glyphBuffer + chr * font -> header -> charSize;

    for(u32 i = 0; i < font -> header -> charSize; i++) { // this assumes 1 row = 1 byte
        for(u32 j = 0; j < 8; j++) {
            if(glyph[i] >> (7 - j) & 1 != 0)
                SetPixel(fb, x + j, y + i, color);
        }
    }
}

void DrawString(FrameBuffer *fb, PSFFont *font, String str, u32 x, u32 y, u32 color) {
    for(u32 i = 0; str[i] != '\0'; i++) {
        DrawChar(fb, font, str[i], x, y, color);
        x += 8;
    }
}