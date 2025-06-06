#include "gnu-efi/inc/efi.h"
#include "gnu-efi/inc/efilib.h"

#include "types.h"
#include "graphics/gop.h"
#include "file-formats/psf.h"
#include "memory/map.h"
// #include "file-formats/bex.h"

#define ERROR_SUCCESS          0x0000
#define ERROR_MEM_ALLOC_FAIL   0x0001
#define ERROR_DEVICE_OPEN_FAIL 0x0002
#define ERROR_DEVICE_READ_FAIL 0x0003
#define ERROR_PROTOCOL_MISSING 0x0004

EFI_HANDLE ImgHdl;
EFI_SYSTEM_TABLE *SysTbl;

FrameBuffer *EFIAPI InitializeGOP() {
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    if(SysTbl -> BootServices -> LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (void **) &gop) != EFI_SUCCESS)
        return NULL;
    
    FrameBuffer *fb;
    SysTbl -> BootServices -> AllocatePool(EfiLoaderData, sizeof(FrameBuffer), (void **) &fb);

    fb -> BaseAddr      = (void *) gop -> Mode ->         FrameBufferBase;
    fb -> BufferSize    =          gop -> Mode ->         FrameBufferSize;
    fb -> Width         =          gop -> Mode -> Info -> HorizontalResolution;
    fb -> Height        =          gop -> Mode -> Info -> VerticalResolution;
    fb -> PixelsPerScan =          gop -> Mode -> Info -> PixelsPerScanLine;

    return fb;
}

EFI_FILE *EFIAPI LoadDirectory(EFI_FILE *parentDirectory, wString path) {
    EFI_LOADED_IMAGE_PROTOCOL *loadedImage;
    SysTbl -> BootServices -> HandleProtocol(
        ImgHdl,
        &gEfiLoadedImageProtocolGuid,
        (void **) &loadedImage
    );

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fileSystem;
    SysTbl -> BootServices -> HandleProtocol(
        loadedImage -> DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (void **) &fileSystem
    );

    if(parentDirectory == NULL) {
        if(fileSystem -> OpenVolume(fileSystem, &parentDirectory) != EFI_SUCCESS)
            return NULL;
    }

    EFI_FILE *file;
    if(parentDirectory -> Open(parentDirectory, &file, path, EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY) != EFI_SUCCESS)
        return NULL;

    return file;
}

EFI_FILE *EFIAPI LoadFile(EFI_FILE *directory, wString filename) {
    EFI_LOADED_IMAGE_PROTOCOL *loadedImage;
    SysTbl -> BootServices -> HandleProtocol(
        ImgHdl,
        &gEfiLoadedImageProtocolGuid,
        (void **) &loadedImage
    );

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fileSystem;
    SysTbl -> BootServices -> HandleProtocol(
        loadedImage -> DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (void **) &fileSystem
    );

    if(directory == NULL) {
        if(fileSystem -> OpenVolume(fileSystem, &directory) != EFI_SUCCESS)
            return NULL;
    }

    EFI_FILE *file;
    if(directory -> Open(directory, &file, filename, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY) != EFI_SUCCESS)
        return NULL;
    
    return file;
}

u64 EFIAPI GetFileSize(EFI_FILE *file) {
    u64 infoSize = 0;
    EFI_FILE_INFO *fileInfo;

    file -> GetInfo(file, &gEfiFileInfoGuid, &infoSize, NULL);
    SysTbl -> BootServices -> AllocatePool(EfiLoaderData, infoSize, (void **) &fileInfo);

    file -> GetInfo(file, &gEfiFileInfoGuid, &infoSize, fileInfo);
    u64 fileSize = fileInfo -> FileSize;

    SysTbl -> BootServices -> FreePool(fileInfo);
    return fileSize;
}

EFI_STATUS ExitWithError(u16 code) {
    const EFI_STATUS statusList[] = {
        EFI_SUCCESS,
        EFI_OUT_OF_RESOURCES,
        EFI_LOAD_ERROR,
        EFI_INVALID_PARAMETER,
        EFI_PROTOCOL_ERROR
    };

    const wString errorList[] = {
        L"\r\n\r\nNo error, execution completed successfully!\r\n",
        L"\r\n\r\nMemory allocation fail!\r\n",
        L"\r\n\r\nDevice open fail!\r\n",
        L"\r\n\r\nDevice read fail!\r\n",
        L"\r\n\r\nProtocol missing!\r\n"
    };

    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, errorList[code]);
    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L"Press any key to continue.\r\n");

    SysTbl -> ConIn -> Reset(SysTbl -> ConIn, FALSE);

    EFI_INPUT_KEY key;
    while(SysTbl -> ConIn -> ReadKeyStroke(SysTbl -> ConIn, &key) == EFI_NOT_READY);

    return statusList[code];
}

// int memcmp(const void *s1, const void *s2, size_t n) {
//     const byte *p1 = s1;
//     const byte *p2 = s2;

//     while(n--) {
//         if(*p1 != *p2)
//             return *p1 - *p2;

//         p1++;
//         p2++;
//     }

//     return 0;
// }

// u16 RelocateFile(u64 *relocationTable, BEFileHeader *header, BESegmentHeader **segmentHeaders, void **segmentData) {
//     u64 relocationCount = *(u64 *) relocationTable;
//     BERelocationEntry *entry = (BERelocationEntry *) ((byte *) relocationTable + sizeof(u64));

//     for(u64 i = 0; i < relocationCount; i++) {
//         if(entry -> flags & BEX_RELOCATION_EXTERNAL != 0)
//             return ERROR_PROTOCOL_MISSING;
        
//         if(entry -> flags & BEX_RELOCATION_ABSOLUTE == 0)
//             return ERROR_PROTOCOL_MISSING;
        
//         switch(entry -> size) {
//             case 8: {
//                 *(qword *) ((byte *) segmentData[entry -> segmentIdx] + entry -> offset) +=
//                     (qword) segmentData[entry -> segmentIdx] - header -> baseAddress;
                
//                 break;
//             }

//             default:
//                 return ERROR_PROTOCOL_MISSING;
//         }

//         entry++;
//     }

//     return ERROR_SUCCESS;
// }

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    ImgHdl = ImageHandle;
    SysTbl = SystemTable;

    SysTbl -> ConOut -> ClearScreen(SysTbl -> ConOut);
    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L"Loading kernel.");

    EFI_FILE *file = LoadFile(NULL, L"stxkrnl.bin");
    if(file == NULL) return ExitWithError(ERROR_DEVICE_OPEN_FAIL);

    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");

    byte *fileData;
    u64 fileSize = GetFileSize(file);
    if(SysTbl -> BootServices -> AllocatePool(EfiLoaderData, fileSize, (void **) &fileData) != EFI_SUCCESS)
        return ExitWithError(ERROR_MEM_ALLOC_FAIL);
    
    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");

    if(file -> Read(file, &fileSize, (void *) fileData))
        return ExitWithError(ERROR_DEVICE_READ_FAIL);

    file -> Close(file);

    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".\r\nKernel loaded!\r\n");
    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L"Wait hold on, last minute fetch quest.");

    void (*kernel_entry)(FrameBuffer *, PSFFont *, MemoryMap *) = (void *) fileData;

    FrameBuffer *fb = InitializeGOP();
    if(fb == NULL) return ExitWithError(ERROR_PROTOCOL_MISSING);

    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");

    EFI_FILE *resourcesFolder = LoadDirectory(NULL, L"resources");
    if(resourcesFolder == NULL) return ExitWithError(ERROR_DEVICE_OPEN_FAIL);

    EFI_FILE *fontsFolder = LoadDirectory(resourcesFolder, L"fonts");
    if(fontsFolder == NULL) return ExitWithError(ERROR_DEVICE_OPEN_FAIL);

    EFI_FILE *fontFile = LoadFile(fontsFolder, L"L8x16-ext.psf");
    if(fontFile == NULL) return ExitWithError(ERROR_DEVICE_OPEN_FAIL);

    u64 fontFileSize = GetFileSize(fontFile);
    PSFFont *font = LoadFont(SysTbl, fontFile, fontFileSize);
    if(font == NULL) return ExitWithError(ERROR_PROTOCOL_MISSING);

    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");

    resourcesFolder -> Close(resourcesFolder);
    fontsFolder -> Close(fontsFolder);
    fontFile -> Close(fontFile);

    MemoryMap *memory = NULL;
    if(SysTbl -> BootServices -> AllocatePool(EfiLoaderData, sizeof(MemoryMap), (void **) &memory) != EFI_SUCCESS)
        return ExitWithError(ERROR_MEM_ALLOC_FAIL);

    memory -> map = NULL;
    memory -> mapSize = 0;
    memory -> descriptorSize = 0;

    u64 mapKey;
    u32 descVer;

    SysTbl -> BootServices -> GetMemoryMap(
        &memory -> mapSize,
        (EFI_MEMORY_DESCRIPTOR *) memory -> map,
        &mapKey,
        &memory -> descriptorSize,
        &descVer
    );

    if(SysTbl -> BootServices -> AllocatePool(EfiLoaderData, memory -> mapSize, (void **) &memory -> map) != EFI_SUCCESS)
        return ExitWithError(ERROR_MEM_ALLOC_FAIL);

    if(SysTbl -> BootServices -> GetMemoryMap(
        &memory -> mapSize,
        (EFI_MEMORY_DESCRIPTOR *) memory -> map,
        &mapKey,
        &memory -> descriptorSize,
        &descVer
    ) != EFI_SUCCESS) return ExitWithError(ERROR_PROTOCOL_MISSING);

    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".\r\nFinished last minute fetch quest!\r\n");
    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L"Jumping to kernel...\r\n");

    SysTbl -> BootServices -> ExitBootServices(ImageHandle, mapKey);
    kernel_entry(fb, font, memory);
    
    SysTbl -> RuntimeServices -> ResetSystem(EfiResetCold, EFI_ABORTED, 0, NULL);
    return EFI_SUCCESS; // not really a success if the kernel returned

    /*==============================================================================
     * All the code below this comment is for loading the kernel from a .bex file.
     * It isn't used because I haven't finished making the linker for .bex files.
     * For now, the kernel is loaded from a raw binary. Which is just sooooo safe.
     *============================================================================*/

    // EFI_FILE *file = LoadFile(NULL, L"stxkrnl.bex");
    // if(file == NULL) return ExitWithError(ERROR_DEVICE_OPEN_FAIL);

    // SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");

    // u64 headerSize = sizeof(BEFileHeader);
    // BEFileHeader *bexHeader;
    // if(SysTbl -> BootServices -> AllocatePool(EfiLoaderData, headerSize, (void **) &bexHeader) != EFI_SUCCESS)
    //     return ExitWithError(ERROR_MEM_ALLOC_FAIL);

    // if(file -> Read(file, &headerSize, (void *) bexHeader) != EFI_SUCCESS)
    //     return ExitWithError(ERROR_DEVICE_READ_FAIL);

    // SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");

    // if(
    //     bexHeader -> signature != BEX_SIGNATURE ||
    //     bexHeader -> version != BEX_VERSION ||
    //     bexHeader -> flags & BEX_HEADER_LIBRAY != 0
    // ) return ExitWithError(ERROR_PROTOCOL_MISSING);

    // SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");

    // BESegmentHeader **bexSegments;
    // if(SysTbl -> BootServices -> AllocatePool(EfiLoaderData, bexHeader -> segmentCount * sizeof(BESegmentHeader), (void **) &bexSegments) != EFI_SUCCESS)
    //     return ExitWithError(ERROR_MEM_ALLOC_FAIL);

    // void **bexData;
    // if(SysTbl -> BootServices -> AllocatePool(EfiLoaderData, bexHeader -> segmentCount * sizeof(void *), (void **) &bexData) != EFI_SUCCESS)
    //     return ExitWithError(ERROR_MEM_ALLOC_FAIL);

    // for(byte i = 0; i < bexHeader -> segmentCount; i++) {
    //     u64 segmentHeaderSize = sizeof(BESegmentHeader);
    //     BESegmentHeader *bexSegmentHeader;
    //     if(SysTbl -> BootServices -> AllocatePool(EfiLoaderData, segmentHeaderSize, (void **) &bexSegmentHeader) != EFI_SUCCESS)
    //         return ExitWithError(ERROR_MEM_ALLOC_FAIL);
        
    //     bexSegments[i] = bexSegmentHeader;
    //     if(file -> Read(file, &segmentHeaderSize, (void *) bexSegmentHeader) != EFI_SUCCESS)
    //         return ExitWithError(ERROR_DEVICE_READ_FAIL);

    //     SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");
    // }

    // for(byte i = 0; i < bexHeader -> segmentCount; i++) {
    //     BESegmentHeader *bexSegmentHeader = bexSegments[i];

    //     byte *segmentData;
    //     u64 segmentSize = bexSegmentHeader -> size;
    //     if(SysTbl -> BootServices -> AllocatePool(EfiLoaderData, segmentSize, (void **) &segmentData) != EFI_SUCCESS)
    //         return ExitWithError(ERROR_MEM_ALLOC_FAIL);

    //     file -> SetPosition(file, bexSegmentHeader -> fileOffset);
    //     if(file -> Read(file, &segmentSize, (void *) segmentData) != EFI_SUCCESS)
    //         return ExitWithError(ERROR_DEVICE_READ_FAIL);

    //     SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");
    // }

    // void (*kernelEntry)() = NULL;

    // for(byte i = 0; i < bexHeader -> segmentCount; i++) {
    //     BESegmentHeader *segmentHeader = bexSegments[i];

    //     if(!memcmp(segmentHeader -> name, ".code", 6))
    //         kernelEntry = (void *) ((qword) bexData[i] + bexHeader -> entryPoint);

    //     if(!memcmp(segmentHeader -> name, ".reloc", 7)) {
    //         u16 errCode = RelocateFile(bexData[i], bexHeader, bexSegments, bexData);
    //         if(errCode != ERROR_SUCCESS) return ExitWithError(errCode);
    //     }
    // }

    // SysTbl -> BootServices -> ExitBootServices(ImageHandle, 0);
    // kernelEntry();
    
    // return EFI_SUCCESS;
}