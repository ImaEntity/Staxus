#include "types.h"
#include "graphics/gop.h"
#include "file-formats/psf.h"
#include "io/print.h"

void kernel_entry(FrameBuffer *fb, PSFFont *defaultFont) {
    InitializePrint(fb, defaultFont);
    ClearScreen(fb, 0); // black

    // DrawString(fb, defaultFont, L"Hello, world!", 0, 0, 0x999999);
    // DrawString(fb, defaultFont, L"Привет, мир!", 0, 16, 0x999999);

    printf(L"Hello, world!\r\nПривет, мир!\r\n");
}