#include <types.h>
#include <video/gop.h>
#include <file-formats/psf.h>
#include <video/print.h>
#include <memory/alloc.h>
#include <memory/managers.h>
#include <memory/utils.h>
#include <storage/ahci.h>
#include <storage/partition/partition.h>
#include <storage/partition/mbr.h>
#include <storage/partition/gpt.h>
#include <file-systems/fat32.h>
#include <interrupt/interrupt.h>
#include <interrupt/gdt.h>
#include <string/utils.h>
#include <stdarg.h>

typedef struct {
    u64 partitionStart;
    u64 partitionSize;
    u8 partitionGUID[16];
    u32 paritionNumber;
} BootDeviceInfo;

void printDir(String tabs, String path);
void toHumanReadable(String result, u64 bytes);
void log(String fmt, ...);
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

void kernelMain(
    FrameBuffer *fb,
    PSFFont *defaultFont,
    MemoryMap *memoryMap,
    BootDeviceInfo *bootDevice
) {
    ClearScreen(fb, 0); // black
    InitializePrint(fb, defaultFont);

    byte shell[] = {
        0xFA,                                                       // cli
        0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, handleNullJump
        0xFF, 0xE0                                                  // jmp rax
    }; *(u64 *) (shell + 3) = (u64) handleNullJump;
    memcpy((void *) 0, shell, sizeof(shell));

    frame = fb;
    defFont = defaultFont;

    printf("kernel base check: %p\n", (u64) kernel_entry);

    log("Hello, world!\n");

    // // Initialize interupts
    InitializeInterrupts();
    // printf("init interrupts\n");

    PIT_SetFrequency(1000);
    // printf("set freq\n");

    RegisterIRQ(0, (void *) TickPIT);
    // printf("registered irq\n");

    // // Initialize GDT
    InitializeGDT();

    FinalizeInterrupts();
    // printf("finalized interrupts\n");

    asm volatile("sti");
    // printf("enabled interrupts\n");

    if(!LoadMemoryManager(IDENTITY_ALLOCATOR, memoryMap, kernel_entry)) {
        log("Failed to initialize memory manager\n");
        while(1);
    }

    char tmp[100];
    toHumanReadable(tmp, GetUsableMemory());
    log("Initalized %s of usable memory\n", tmp);
    
    RegisterGPTPartitionController();
    RegisterMBRPartitionController();

    RegisterFAT32FileSystem();

    // Initialize keyboard / mouse

    // Initialize storage devices
    AHCIController controller = FindAHCIController();
    if(controller.baseAddrReg != 0) InitializeAHCIController(&controller);

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
        log("Found partition %d: %s\n", i, sizeStr);
    }

    // mkdir("/krnlstate");

    printDir("", "/");
    log("\n");

    // Initialize GPU?

    toHumanReadable(tmp, GetUsableMemory() - GetAvailableMemory());
    log("Using %s of memory after initalizing core systems\n", tmp);

    while(1);
}

void toHumanReadable(String result, u64 bytes) {
    String suffixs[] = {"B", "KB", "MB", "GB", "TB"};
    u64 suffixIndex = 0;

    double b = (double) bytes;
    while(b >= 1024 && suffixIndex < 5) {
        b /= 1024;
        suffixIndex++;
    }

    sprintf(result, "%.2f %s", b, suffixs[suffixIndex]);
}

void printDir(String tabs, String path) {
    Dir *dir = opendir(path);
    if(dir == NULL) {
        log("failed: %s\n", path);
        return;
    }

    log("%s%s\n", tabs, path);

    String newTabs = malloc(strlen(tabs) + 5);
    strcpy(newTabs, tabs); strcat(newTabs, "    ");

    DirEntry *ent;
    while((ent = readdir(dir)) != NULL) {
        if(strcmp(ent -> name, ".") == 0 || strcmp(ent -> name, "..") == 0)
            continue;
        
        u64 pLen = strlen(path);
        String fullPath = malloc(pLen + strlen(ent->name) + 2);
        strcpy(fullPath, path);
        if(path[pLen - 1] != '/') strcat(fullPath, "/");
        strcat(fullPath, ent -> name);

        if(ent -> type == DIR_ENTRY_DIR) {
            printDir(newTabs, fullPath);
        } else {
            char buf[100];
            toHumanReadable(buf, ent -> size);

            log("%s%s (%s)\n", newTabs, ent -> name, buf);
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

void log(String fmt, ...) {
    va_list args;
    va_start(args, fmt);

    static char buf[1024];
    memset(buf, 0, sizeof(buf));

    vsprintf(buf, fmt, args);
    
    printf(buf);
    // if(!exists("/krnlstate")) return;

    // FILE *fp = fopen("/krnlstate/setup.log", "a");
    // fprintf(fp, buf); fclose(fp);
}

void TickPIT() {
    printf("hgit\n");
}

FrameBuffer *getFrameBuffer() {
    return frame;
}

PSFFont *getDefaultFont() {
    return defFont;
}