#include "inc/efi.h"
#include "inc/efilib.h"

#include "include/bex.h"

#define ERROR_SUCCESS           0xFFFF
#define ERROR_MEM_ALLOC_FAIL    0x0000
#define ERROR_DEVICE_OPEN_FAIL  0x0001
#define ERROR_DEVICE_READ_FAIL  0x0002
#define ERROR_PROTOCOL_MISMATCH 0x0003

EFI_HANDLE ImgHdl;
EFI_SYSTEM_TABLE *SysTbl;

EFI_FILE *EFIAPI LoadFile(EFI_FILE *directory, CHAR16 *filename) {
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

    if(directory == NULL)
        fileSystem -> OpenVolume(fileSystem, &directory);

    EFI_FILE *file;
    EFI_STATUS status = directory -> Open(directory, &file, filename, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    if(status != EFI_SUCCESS)
        return NULL;
    
    return file;
}

EFI_STATUS ExitWithError(u16 code) {
    const EFI_STATUS statusList[] = {
        EFI_OUT_OF_RESOURCES,
        EFI_LOAD_ERROR,
        EFI_INVALID_PARAMETER,
        EFI_PROTOCOL_ERROR
    };

    const WString errorList[] = {
        L"\r\n\r\nMemory allocation fail!\r\n",
        L"\r\n\r\nDevice open fail!\r\n",
        L"\r\n\r\nDevice read fail!\r\n",
        L"\r\n\r\nProtocol mismatch!\r\n"
    };

    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, errorList[code]);
    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L"Press any key to continue.\r\n");

    SysTbl -> ConIn -> Reset(SysTbl -> ConIn, FALSE);

    EFI_INPUT_KEY key;
    while(SysTbl -> ConIn -> ReadKeyStroke(SysTbl -> ConIn, &key) == EFI_NOT_READY);

    return statusList[code];
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const byte *p1 = s1;
    const byte *p2 = s2;

    while(n--) {
        if(*p1 != *p2)
            return *p1 - *p2;

        p1++;
        p2++;
    }

    return 0;
}

u16 RelocateFile(u64 *relocationTable, BEFileHeader *header, BESegmentHeader **segmentHeaders, void **segmentData) {
    u64 relocationCount = *(u64 *) relocationTable;
    BERelocationEntry *entry = (BERelocationEntry *) ((byte *) relocationTable + sizeof(u64));

    for(u64 i = 0; i < relocationCount; i++) {
        if(entry -> flags & BEX_RELOCATION_EXTERNAL != 0)
            return ERROR_PROTOCOL_MISMATCH;
        
        if(entry -> flags & BEX_RELOCATION_ABSOLUTE == 0)
            return ERROR_PROTOCOL_MISMATCH;
        
        switch(entry -> size) {
            case 8: {
                *(qword *) ((byte *) segmentData[entry -> segmentIdx] + entry -> offset) +=
                    (qword) segmentData[entry -> segmentIdx] - header -> baseAddress;
                
                break;
            }

            default:
                return ERROR_PROTOCOL_MISMATCH;
        }

        entry++;
    }

    return ERROR_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    ImgHdl = ImageHandle;
    SysTbl = SystemTable;

    SysTbl -> ConOut -> ClearScreen(SysTbl -> ConOut);
    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L"Loading kernel.");

    EFI_FILE *file = LoadFile(NULL, L"lnrknrl.bex");
    if(file == NULL) return ExitWithError(ERROR_DEVICE_OPEN_FAIL);

    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");

    u64 headerSize = sizeof(BEFileHeader);
    BEFileHeader *bexHeader;
    if(SysTbl -> BootServices -> AllocatePool(EfiLoaderData, headerSize, (void **) &bexHeader) != EFI_SUCCESS)
        return ExitWithError(ERROR_MEM_ALLOC_FAIL);

    if(file -> Read(file, &headerSize, (void *) bexHeader) != EFI_SUCCESS)
        return ExitWithError(ERROR_DEVICE_READ_FAIL);

    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");

    if(
        bexHeader -> signature != BEX_SIGNATURE ||
        bexHeader -> version != BEX_VERSION ||
        bexHeader -> flags & BEX_HEADER_LIBRAY != 0
    ) return ExitWithError(ERROR_PROTOCOL_MISMATCH);

    SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");

    BESegmentHeader **bexSegments;
    if(SysTbl -> BootServices -> AllocatePool(EfiLoaderData, bexHeader -> segmentCount * sizeof(BESegmentHeader), (void **) &bexSegments) != EFI_SUCCESS)
        return ExitWithError(ERROR_MEM_ALLOC_FAIL);

    void **bexData;
    if(SysTbl -> BootServices -> AllocatePool(EfiLoaderData, bexHeader -> segmentCount * sizeof(void *), (void **) &bexData) != EFI_SUCCESS)
        return ExitWithError(ERROR_MEM_ALLOC_FAIL);

    for(byte i = 0; i < bexHeader -> segmentCount; i++) {
        u64 segmentHeaderSize = sizeof(BESegmentHeader);
        BESegmentHeader *bexSegmentHeader;
        if(SysTbl -> BootServices -> AllocatePool(EfiLoaderData, segmentHeaderSize, (void **) &bexSegmentHeader) != EFI_SUCCESS)
            return ExitWithError(ERROR_MEM_ALLOC_FAIL);
        
        bexSegments[i] = bexSegmentHeader;
        if(file -> Read(file, &segmentHeaderSize, (void *) bexSegmentHeader) != EFI_SUCCESS)
            return ExitWithError(ERROR_DEVICE_READ_FAIL);

        SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");
    }

    for(byte i = 0; i < bexHeader -> segmentCount; i++) {
        BESegmentHeader *bexSegmentHeader = bexSegments[i];

        byte *segmentData;
        u64 segmentSize = bexSegmentHeader -> size;
        if(SysTbl -> BootServices -> AllocatePool(EfiLoaderData, segmentSize, (void **) &segmentData) != EFI_SUCCESS)
            return ExitWithError(ERROR_MEM_ALLOC_FAIL);

        file -> SetPosition(file, bexSegmentHeader -> fileOffset);
        if(file -> Read(file, &segmentSize, (void *) segmentData) != EFI_SUCCESS)
            return ExitWithError(ERROR_DEVICE_READ_FAIL);

        SysTbl -> ConOut -> OutputString(SysTbl -> ConOut, L".");
    }

    void (*kernelEntry)() = NULL;

    for(byte i = 0; i < bexHeader -> segmentCount; i++) {
        BESegmentHeader *segmentHeader = bexSegments[i];

        if(!memcmp(segmentHeader -> name, ".code", 6))
            kernelEntry = (void *) ((qword) bexData[i] + bexHeader -> entryPoint);

        if(!memcmp(segmentHeader -> name, ".reloc", 7)) {
            u16 errCode = RelocateFile(bexData[i], bexHeader, bexSegments, bexData);
            if(errCode != ERROR_SUCCESS) return ExitWithError(errCode);
        }
    }

    SysTbl -> BootServices -> ExitBootServices(ImageHandle, 0);
    kernelEntry();
    
    return EFI_SUCCESS;
}