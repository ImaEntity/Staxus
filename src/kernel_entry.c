#include <types.h>
#include <video/gop.h>
#include <video/print.h>
#include <file-formats/pe.h>
#include <file-formats/psf.h>
#include <file-systems/fat32.h>
#include <memory/alloc.h>
#include <memory/managers.h>
#include <memory/utils.h>
#include <storage/ahci.h>
#include <storage/partition/partition.h>
#include <storage/partition/mbr.h>
#include <storage/partition/gpt.h>
#include <interrupt/interrupt.h>
#include <interrupt/gdt.h>
#include <string/utils.h>
#include <io/keyboard.h>
#include <io/serial.h>
#include <io/mouse.h>
#include <math/math.h>
#include <stdarg.h>

typedef struct {
    u64 partitionStart;
    u64 partitionSize;
    u8 partitionGUID[16];
    u32 paritionNumber;
} BootDeviceInfo;

void printDir(String tabs, String path);
void toHumanReadable(String result, u64 bytes);
void dbg(String fmt, ...);
void handleNullJump();

void PIT_SetFrequency(u32 hertz);
void TickPIT();

static FrameBuffer *frame;
static PSFFont *defFont;

FrameBuffer *getFrameBuffer();
PSFFont *getDefaultFont();

static byte stack[1024 * 1024];
void kernelExit();
__attribute((naked)) void kernel_entry() {
    asm volatile("movq %0, %%rsp" :: "r"((qword) (stack + sizeof(stack) - 1024) & ~0xF));
    asm volatile("pushq %0" :: "r"((qword) kernelExit));
    asm volatile("jmp kernelMain");
}

void kernelExit() {
    asm volatile("cli");

    PSFFont *font = LoadFont("/resources/fonts/D8x16-ext.psf");
    if(font == NULL) font = defFont;

    DrawString(
        frame, font,
        L"Invalid kernel exit!",
        frame -> Width / 2 - (20 * 8) / 2,
        frame -> Height / 2 - font -> header -> charSize / 2,
        0xFF0000
    );

    while(1) asm volatile("hlt");
}

void OnMouseEvent(MouseEvent event) {
    DrawLine(frame, event.prevX, event.prevY, event.curX, event.curY, 0x0000FF);
    printf("Mouse event %x %x %x %x %d %x %x", event.prevX, event.prevY, event.curX, event.curY, event.scrollAmount, event.prevButtons, event.buttons);
}

void kernelMain(
    FrameBuffer *fb,
    PSFFont *defaultFont,
    MemoryMap *memoryMap,
    BootDeviceInfo *bootDevice
) {
    ClearScreen(fb, 0); // black
    InitializePrint(fb, defaultFont);

    *(void **) 0x1000000 = fb;
    *(void **) 0x1000008 = defaultFont;

    byte shell[] = {
        0xFA,                                                       // cli
        0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, handleNullJump
        0xFF, 0xE0                                                  // jmp rax
    }; *(u64 *) (shell + 3) = (u64) handleNullJump;
    memcpy((void *) 0, shell, sizeof(shell));

    frame = fb;
    defFont = defaultFont;

    dbg("Hello, world!\n");

    InitializeGDT(((u64) stack + sizeof(stack) - 1024) & ~0xF);
    if(!InitializeInterrupts()) {
        dbg("No APIC present on system\n");
        while(1);
    }

    InitializeSerial(COM1, NULL);

    dbg("Initialized screen as %u x %u\n", fb -> Width, fb -> Height);
    if(!LoadMemoryManager(IDENTITY_ALLOCATOR, memoryMap, kernel_entry)) {
        dbg("Failed to initialize memory manager\n");
        while(1);
    }

    char tmp[100];
    toHumanReadable(tmp, GetUseableMemory());
    dbg("Initialized %s of useable memory\n", tmp);
    
    RegisterGPTPartitionController();
    RegisterMBRPartitionController();

    RegisterFAT32FileSystem();

    // Initialize keyboard / mouse
    // dbg("Initialized mouse with ID %u\n", InitializeMouse(OnMouseEvent));

    // Initialize storage devices
    AHCIController controller = FindAHCIController();
    if(controller.baseAddrReg != 0) {
        dbg("Found AHCI controller\n");
        u8 ahciCount = InitializeAHCIController(&controller);
        dbg("Registered %d AHCI block devices\n", ahciCount);
    }

    u16 count;
    BlockDevice **devices = GetBlockDevices(&count);
    for(u16 i = 0; i < count; i++) {
        BlockDevice *dev = devices[i];
        if((dev -> flags & BLOCK_DEVICE_FLAG_PHYSICAL) != 0)
            RegisterPartitionBlocks(dev);
    }

    Partition **parts = GetPartitions(&count);
    for(u16 i = 0; i < count; i++) {
        Partition *part = parts[i];

        if(
            part -> lbaStart == bootDevice -> partitionStart &&
            part -> lbaEnd - part -> lbaStart + 1 == bootDevice -> partitionSize
        ) MountFileSystem(part -> device, "/");

        char sizeStr[100];
        toHumanReadable(sizeStr, part -> device -> sectorCount * part -> device -> sectorSize);
        dbg("Found partition %d: %s\n", i, sizeStr);
    }

    printDir("", "/");
    dbg("\n");

    // Initialize GPU?

    PEModule exe = LoadPEFile("/resources", "test_pe.exe");
    void (*entry)() = GetPEEntry(exe);
    entry(fb, defaultFont);
    UnloadPEFile(exe);

    toHumanReadable(tmp, GetUseableMemory() - GetAvailableMemory());
    dbg("Using %s of memory after initalizing core systems\n", tmp);

    asm volatile("int $0x2c");

    while(1);
}

void toHumanReadable(String result, u64 bytes) {
    u64 suffixIndex = 0;
    String suffixs[] = {
        "B", "KB", "MB", "GB",
        "TB", "PB", "EB", "ZB"
    };

    double b = (double) bytes;
    while(b >= 1024 && suffixIndex < 7) {
        b /= 1024;
        suffixIndex++;
    }

    sprintf(result, "%.2f %s", b, suffixs[suffixIndex]);
}

void printDir(String tabs, String path) {
    Dir *dir = opendir(path);
    if(dir == NULL) {
        dbg("failed: %s\n", path);
        return;
    }

    dbg("%s%s\n", tabs, path);

    String newTabs = malloc(strlen(tabs) + 5);
    strcpy(newTabs, tabs); strcat(newTabs, "    ");

    DirEntry *ent;
    while((ent = readdir(dir)) != NULL) {
        if(strcmp(ent -> name, ".") == 0 || strcmp(ent -> name, "..") == 0)
            continue;
        
        u64 pLen = strlen(path);
        String fullPath = malloc(pLen + strlen(ent -> name) + 2);
        strcpy(fullPath, path);
        if(path[pLen - 1] != '/') strcat(fullPath, "/");
        strcat(fullPath, ent -> name);

        if(ent -> type == DIR_ENTRY_DIR) {
            printDir(newTabs, fullPath);
        } else {
            char buf[100];
            toHumanReadable(buf, ent -> size);

            dbg("%s%s (%s)\n", newTabs, ent -> name, buf);
        }

        free(fullPath);
    }

    closedir(dir);
}

static inline void outb(u16 port, u8 value) {
    asm volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

void PIT_SetFrequency(u32 hertz) {
    u32 divisor = 1193180 / hertz;

    outb(0x43, 0x36);
    outb(0x40,  divisor       & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

void handleNullJump() {
    PSFFont *font = LoadFont("/resources/fonts/D8x16-ext.psf");
    if(font == NULL) font = defFont;

    DrawString(
        frame, font,
        L"Attempt to call a null pointer!",
        frame -> Width / 2 - (31 * 8) / 2,
        frame -> Height / 2 - font -> header -> charSize / 2,
        0xFF0000
    );

    while(1);
}

void dbg(String fmt, ...) {
    va_list args;
    va_start(args, fmt);

    static char buf[1024];
    memset(buf, 0, sizeof(buf));

    int len = vsprintf(buf, fmt, args);
    va_end(args);
    
    printf(buf);
    if(SerialActive()) WriteSerial((byte *) buf, len);
}

static f64 g = 0;
static int ticks = 0;
void TickPIT() {
    printf("Got interrupt!\n");
    
    ticks++;
    if(ticks < 10) return;

    u32 cx = frame -> Width / 2;
    u32 cy = frame -> Height / 2;
    i32 r = 250;
    i32 s = 100;

    for(i32 y = -r; y <= r; y++) {
        for(i32 x = -r; x <= r; x++) {
            SetPixel(frame, cx + x, cy + y, 0);

            f64 ang = atan2(y, x) + PI / 2;
            if(ang < 0) ang += 2 * PI;
            if(ang > 2 * PI) ang -= 2 * PI;
            ang /= 2 * PI;

            if(ang > g) continue;

            if(x * x + y * y > r * r) continue;
            if(x * x + y * y < (r - s) * (r - s)) continue;

            u32 col = 0xFF00FF;
            if((((x >> 5) + (y >> 5)) & 1) == 0) col = 0x000000;
            if(x * x + y * y > (r - 2) * (r - 2)) col = 0xFFFFFF;
            if(x * x + y * y < (r - s + 2) * (r - s + 2)) col = 0xFFFFFF;
            if(ang + 0.001 > g || ang < 0.001) col = 0xFFFFFF;

            SetPixel(frame, cx + x, cy + y, col);
        }
    }

    g += 0.01;
    ticks = 0;
}

FrameBuffer *getFrameBuffer() {
    return frame;
}

PSFFont *getDefaultFont() {
    return defFont;
}