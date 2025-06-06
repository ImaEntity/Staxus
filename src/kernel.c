#include "types.h"
#include "graphics/gop.h"
#include "file-formats/psf.h"
#include "io/print.h"
#include "memory/alloc.h"

void kernel_entry(FrameBuffer *fb, PSFFont *defaultFont, MemoryMap *memory) {
    InitializePrint(fb, defaultFont);
    u64 initBlocks = InitializeMemory(memory);
    ClearScreen(fb, 0); // black    

    printf(L"Hello, world!\r\n");
    printf(L"Initialized %llu blocks of memory\r\n\r\n", initBlocks);

    

    while(1);
}