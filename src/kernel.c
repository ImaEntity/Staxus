#include "types.h"
#include "graphics/gop.h"
#include "file-formats/psf.h"
#include "io/print.h"

void kernel_entry(FrameBuffer *fb, PSFFont *defaultFont) {
    InitializePrint(fb, defaultFont);
    ClearScreen(fb, 0); // black

    printf(L"Hello, world!\r\nПривет, мир!\r\n");
}