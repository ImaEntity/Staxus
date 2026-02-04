#include <types.h>
#include <video/gop.h>
#include <file-formats/psf.h>
#include <video/print.h>
#include <memory/alloc.h>
#include <memory/managers.h>
#include <storage/ahci.h>

void toHumanReadable(wString result, u64 bytes);
void kernel_entry(FrameBuffer *fb, PSFFont *defaultFont, MemoryMap *memoryMap) {
    ClearScreen(fb, 0); // black    
    InitializePrint(fb, defaultFont);

    printf(L"Hello, world!\r\n");

    // Initialize interupts

    if(!LoadMemoryManager(IDENTITY_ALLOCATOR, memoryMap)) {
        printf(L"Failed to initialize memory manager\r\n");
        while(1);
    }

    wchar tmp[100];
    toHumanReadable(tmp, GetUsableMemory());
    printf(L"Initalized %s of usable memory\r\n", tmp);
    
    // Initialize keyboard / mouse

    // Initialize storage devices
    AHCIController controller = FindAHCIController();
    if(controller.baseAddrReg != 0)
        InitializeAHCIController(controller);

    // Initialize GPU?

    while(1);
}

void toHumanReadable(wString result, u64 bytes) {
    wString suffixs[] = {L"B", L"KB", L"MB", L"GB", L"TB"};
    u64 suffixIndex = 0;

    double b = (double) bytes;
    while(b >= 1024 && suffixIndex < 5) {
        b /= 1024;
        suffixIndex++;
    }

    sprintf(result, L"%.2f %s", b, suffixs[suffixIndex]);
}
