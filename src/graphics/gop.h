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

void DrawLine(FrameBuffer *fb, u32 x0, u32 y0, u32 x1, u32 y1, u32 color);

void DrawChar(FrameBuffer *fb, PSFFont *font, wchar chr, u32 x, u32 y, u32 color);
void DrawString(FrameBuffer *fb, PSFFont *font, wString str, u32 x, u32 y, u32 color);

#endif