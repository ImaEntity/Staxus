#include "types.h"
#include "graphics/gop.h"
#include "file-formats/psf.h"

void kernel_entry(FrameBuffer *fb, PSFFont *defaultFont) {
    ClearScreen(fb, 0); // black
    DrawString(fb, defaultFont, "Hello, world!", 0, 0, 0x999999);
}