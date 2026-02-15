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
#include <string/utils.h>
#include <stdarg.h>

void printDir(String tabs, String path);
void toHumanReadable(String result, u64 bytes);
void log(String fmt, ...);
void handleNullJump();

void test_fs();

static FrameBuffer *frame;
static PSFFont *defFont;

void kernel_entry(FrameBuffer *fb, PSFFont *defaultFont, MemoryMap *memoryMap) {
    byte shell[] = {
        0xFA,                                                       // cli
        0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, handleNullJump
        0xFF, 0xE0                                                  // jmp rax
    }; *(u64 *) (shell + 3) = (u64) handleNullJump;
    memcpy((void *) 0, shell, sizeof(shell));

    frame = fb;
    defFont = defaultFont;

    ClearScreen(fb, 0); // black
    InitializePrint(fb, defaultFont);

    log("Hello, world!\n");

    // Initialize interupts

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

    u16 pCount = count;
    devices = GetBlockDevices(&count);

    for(u16 i = 0; i < count; i++) {
        BlockDevice *dev = devices[i];

        if((dev -> flags & BLOCK_DEVICE_FLAG_PHYSICAL) != 0) continue;

        char sizeStr[100];
        toHumanReadable(sizeStr, dev -> sectorCount * dev -> sectorSize);
        log("Found device %d: %ls (%s)\n", i, dev -> model, sizeStr);
    }

    if(
        count > pCount &&
        MountFileSystem(devices[pCount], "/")
    ) log("Found and mounted boot device\n");

    mkdir("/krnlstate");

    // printDir("", "/");
    // log("\n");

    test_fs();

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
    return;

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

void handleNullJump() {
    DrawString(
        frame, defFont,
        L"Attempt to call a null pointer!",
        frame -> Width / 2 - (31 * 8) / 2,
        frame -> Height / 2 - defFont -> header -> charSize / 2,
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

    // FILE *fp = fopen("/krnlstate/log.log", "a");
    // fprintf(fp, buf); fclose(fp);
}

void assert(boolean condition, String failmsg) {
    if(condition) return;

    log(failmsg);
    while(1);
}


void test_fs_extra() {
    log("\n== FAT32 EXTRA TEST ==\n\n");

    int T = 1;
    #define TEST(msg) log("[Test %d]: %s\n", T++, msg)

    static char buf[8192];
    static char cmp[8192];

    // -------------------------------------------------
    TEST("zero-byte file");
    FILE *f = fopen("/test/zero.bin", "w");
    assert(f != NULL, "FAIL: create zero\n");
    fclose(f);

    assert(exists("/test/zero.bin"), "FAIL: zero exists\n");

    f = fopen("/test/zero.bin", "r");
    u64 r = fread(buf, 1, sizeof(buf), f);
    assert(r == 0, "FAIL: zero read not 0\n");
    fclose(f);

    // -------------------------------------------------
    TEST("truncate larger -> smaller");
    f = fopen("/test/trunc.txt", "w");
    for(int i=0;i<1000;i++) fprintf(f, "A");
    fclose(f);

    f = fopen("/test/trunc.txt", "w");
    fprintf(f, "B");
    fclose(f);

    memset(buf, 0, sizeof(buf));
    f = fopen("/test/trunc.txt", "r");
    fread(buf, 1, sizeof(buf), f);
    fclose(f);

    assert(buf[0]=='B' && buf[1]==0, "FAIL: truncate\n");

    // -------------------------------------------------
    TEST("append correctness");
    f = fopen("/test/app.txt", "w");
    fprintf(f, "1");
    fclose(f);

    f = fopen("/test/app.txt", "a");
    fprintf(f, "2");
    fclose(f);

    f = fopen("/test/app.txt", "r");
    fread(buf,1,2,f);
    fclose(f);
    assert(memcmp(buf,"12",2)==0, "FAIL: append order\n");

    // -------------------------------------------------
    TEST("exact cluster boundary");
    memset(cmp, 0xAA, 4096);

    f = fopen("/test/cluster.bin", "w");
    fwrite(cmp,1,4096,f);     // exactly 1 cluster
    fclose(f);

    memset(buf,0,sizeof(buf));
    f = fopen("/test/cluster.bin", "r");
    fread(buf,1,4096,f);
    fclose(f);

    assert(memcmp(buf,cmp,4096)==0, "FAIL: cluster exact\n");

    // -------------------------------------------------
    TEST("cluster + 1 byte");
    f = fopen("/test/cluster2.bin", "w");
    fwrite(cmp,1,4096,f);
    fwrite("Z",1,1,f);
    fclose(f);

    memset(buf,0,sizeof(buf));
    f = fopen("/test/cluster2.bin", "r");
    fread(buf,1,4097,f);
    fclose(f);

    assert(buf[4096]=='Z', "FAIL: cluster chain\n");

    // -------------------------------------------------
    TEST("read past EOF");
    f = fopen("/test/app.txt", "r");
    r = fread(buf,1,100,f);
    fclose(f);
    assert(r == 2, "FAIL: read past EOF size\n");

    // -------------------------------------------------
    TEST("reopen consistency");
    f = fopen("/test/reopen.bin","w");
    memset(cmp,0x55,1024);
    fwrite(cmp,1,1024,f);
    fclose(f);

    f = fopen("/test/reopen.bin","r");
    fread(buf,1,1024,f);
    fclose(f);
    assert(memcmp(buf,cmp,1024)==0,"FAIL: reopen data\n");

    // -------------------------------------------------
    TEST("many small files (200)");
    mkdir("/test/many");

    for(int i=0;i<200;i++) {
        char name[64];
        sprintf(name,"/test/many/f%d.txt",i);
        f = fopen(name,"w");
        fprintf(f,"file%d",i);
        fclose(f);
    }

    for(int i=0;i<200;i++) {
        char name[64];
        sprintf(name,"/test/many/f%d.txt",i);

        char ebuf[128];
        sprintf(ebuf, "FAIL: many exists (%s)\n", name);

        assert(exists(name), ebuf);
    }

    // -------------------------------------------------
    TEST("deep path");
    mkdir("/test/deep");
    mkdir("/test/deep/a");
    mkdir("/test/deep/a/b");
    mkdir("/test/deep/a/b/c");
    mkdir("/test/deep/a/b/c/d");

    f = fopen("/test/deep/a/b/c/d/file.txt","w");
    fprintf(f,"deep");
    fclose(f);

    assert(exists("/test/deep/a/b/c/d/file.txt"),"FAIL: deep path\n");

    // -------------------------------------------------
    TEST("rename overwrite");
    f = fopen("/test/r1.txt","w"); fprintf(f,"1"); fclose(f);
    f = fopen("/test/r2.txt","w"); fprintf(f,"2"); fclose(f);

    rename("/test/r1.txt","/test/r2.txt");
    assert(exists("/test/r1.txt"),"FAIL: rename overwrite\n");

    // -------------------------------------------------
    TEST("delete open file");
    f = fopen("/test/tmp.txt","w");
    fprintf(f,"temp");
    remove("/test/tmp.txt");   // FAT32 allows unlink while open?
    fclose(f);
    assert(!exists("/test/tmp.txt"),"FAIL: delete open\n");

    // -------------------------------------------------
    TEST("1MB random pattern");
    f = fopen("/test/random.bin","w");
    for(int i=0;i<256;i++){
        for(int j=0;j<4096;j++)
            cmp[j] = (char)(i ^ j);
        fwrite(cmp,1,4096,f);
    }
    fclose(f);

    f = fopen("/test/random.bin","r");
    for(int i=0;i<256;i++){
        fread(buf,1,4096,f);
        for(int j=0;j<4096;j++)
            cmp[j] = (char)(i ^ j);

        char pbuf[128];
        sprintf(pbuf, "FAIL: random verify (chunk %d / 256)\n", i + 1);

        assert(memcmp(buf,cmp,4096)==0,pbuf);
    }
    fclose(f);

    log("\n== FAT32 EXTRA DONE ==\n\n");
    #undef TEST
}

void test_fs() {
    int T = 1;
    #define TEST(msg) log("[Test %d]: %s\n", T++, msg)

    log("\n== FAT32 TEST START ==\n\n");

    TEST("mkdir + exists");
    mkdir("/test");
    assert(exists("/test"), "FAIL: mkdir '/test'\n");

    mkdir("/test/sub");
    assert(exists("/test/sub"), "FAIL: mkdir '/test/sub'\n");

    TEST("create + write small file");
    FILE *f = fopen("/test/a.txt", "w");
    assert(f != NULL, "FAIL: fopen\n");

    u64 written = fprintf(f, "hello world\n");

    assert(written == 12, "FAIL: fprintf 'hello, world'\n");
    fclose(f);

    assert(exists("/test/a.txt"), "FAIL: exists '/test/a.txt'\n");

    TEST("read back");
    char buf[64] = {0};
    f = fopen("/test/a.txt", "r");
    fread(buf, 1, sizeof(buf), f);
    fclose(f);
    assert(memcmp(buf, "hello world\n", 13) == 0, "FAIL: fread '/test/a.txt'\n");

    TEST("overwrite file");
    f = fopen("/test/a.txt", "w");
    fprintf(f, "short\n");
    fclose(f);

    TEST("append test");
    f = fopen("/test/a.txt", "a");
    fprintf(f, "append\n");
    fclose(f);

    TEST("multi-cluster file");
    f = fopen("/test/big.bin", "w");
    static char big[4096];
    for(int i=0;i<4096;i++) big[i] = (char)(i & 0xFF);

    for(int i=0;i<128;i++) // ~512 KB
        fwrite(big, 1, sizeof(big), f);

    fclose(f);

    TEST("seek + tell");
    f = fopen("/test/big.bin", "r");
    fseek(f, 10000, 0);
    u64 pos = ftell(f);
    assert(pos == 10000, "FAIL: fseek/ftell\n");

    fread(buf, 1, 16, f);
    rewind(f);
    assert(ftell(f) == 0, "FAIL: rewind\n");
    fclose(f);

    TEST("rename");
    rename("/test/a.txt", "/test/renamed.txt");
    assert(exists("/test/renamed.txt"), "FAIL: rename '/test/renamed.txt'\n");
    assert(!exists("/test/a.txt"), "FAIL: rename '/test/a.txt'\n");

    TEST("listing directory");
    Dir *d = opendir("/test");
    assert(d != NULL, "FAIL: opendir\n");

    int idx = 0;
    String correct[] = {"sub", "big.bin", "renamed.txt"};

    DirEntry *e;
    while((e = readdir(d)))
        assert(strcmp(e -> name, correct[idx++]) == 0, "FAIL: readdir '/test'\n");

    closedir(d);

    TEST("delete file");
    remove("/test/renamed.txt");
    assert(!exists("/test/renamed.txt"), "FAIL: remove '/test/renamed.txt'\n");

    TEST("delete multi-cluster file");
    remove("/test/big.bin");
    assert(!exists("/test/big.bin"), "FAIL: remove '/test/big.bin'\n");

    TEST("nested directories");
    mkdir("/test/sub/a");
    mkdir("/test/sub/a/b");
    mkdir("/test/sub/a/b/c");

    TEST("dirname + basepath");
    String bname = basepath("/test/sub/a/file.txt");
    String dname = dirname("/test/sub/a/file.txt");
    
    assert(strcmp(bname, "file.txt") == 0, "FAIL: basepath\n");
    assert(strcmp(dname, "/test/sub/a") == 0, "FAIL: dirname\n");

    test_fs_extra();

    log("\n== FAT32 TEST END ==\n\n");
    #undef TEST
}