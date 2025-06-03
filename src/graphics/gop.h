#ifndef HH_GOP
#define HH_GOP

#include "../types.h"
#include "../file-formats/psf.h"

typedef struct {
    void *BaseAddr;
    u64 BufferSize;
    u32 Width;
    u32 Height;
    u32 PixelsPerScan;
} FrameBuffer;

void ClearScreen(FrameBuffer *fb, u32 color);

void SetPixel(FrameBuffer *fb, u32 x, u32 y, u32 color);
u32 GetPixel(FrameBuffer *fb, u32 x, u32 y);

void DrawChar(FrameBuffer *fb, PSFFont *font, byte chr, u32 x, u32 y, u32 color);
void DrawString(FrameBuffer *fb, PSFFont *font, String str, u32 x, u32 y, u32 color);

#endif