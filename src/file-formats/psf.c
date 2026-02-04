#include "psf.h"
#include <types.h>

#include <gnu-efi/inc/efi.h>
#include <gnu-efi/inc/efilib.h>

PSFFont *LoadFont(EFI_SYSTEM_TABLE *SysTbl, EFI_FILE *file, u64 fileSize) {
    PSFFont *font;

    SysTbl -> BootServices -> AllocatePool(EfiLoaderData, sizeof(PSFFont), (void **) &font);
    if(font == NULL) return NULL;

    SysTbl -> BootServices -> AllocatePool(EfiLoaderData, sizeof(PSFHeader), (void **) &font -> header);
    if(font -> header == NULL) {
        SysTbl -> BootServices -> FreePool(font);
        return NULL;
    }

    u64 headerSize = sizeof(PSFHeader);
    if(file -> Read(file, &headerSize, font -> header) != EFI_SUCCESS) {
        SysTbl -> BootServices -> FreePool(font -> header);
        SysTbl -> BootServices -> FreePool(font);
        return NULL;
    }

    u64 glyphCount = (font -> header -> fontMode & PSF_MODE512) != 0 ? 512 : 256;
    u64 glyphBufferSize = font -> header -> charSize * glyphCount;

    SysTbl -> BootServices -> AllocatePool(EfiLoaderData, glyphBufferSize, (void **) &font -> glyphBuffer);
    if(font -> glyphBuffer == NULL) {
        SysTbl -> BootServices -> FreePool(font -> header);
        SysTbl -> BootServices -> FreePool(font);
        return NULL;
    }

    file -> SetPosition(file, sizeof(PSFHeader));
    if(file -> Read(file, &glyphBufferSize, font -> glyphBuffer)) {
        SysTbl -> BootServices -> FreePool(font -> glyphBuffer);
        SysTbl -> BootServices -> FreePool(font -> header);
        SysTbl -> BootServices -> FreePool(font);
        return NULL;
    }

    if(font -> header -> magic != PSF_MAGIC) {
        SysTbl -> BootServices -> FreePool(font -> glyphBuffer);
        SysTbl -> BootServices -> FreePool(font -> header);
        SysTbl -> BootServices -> FreePool(font);
        return NULL;
    }

    // no unicode table
    if(fileSize <= sizeof(PSFHeader) + glyphBufferSize)
        return font;
    
    word *inFileTable;
    SysTbl -> BootServices -> AllocatePool(EfiLoaderData, fileSize - sizeof(PSFHeader) - glyphBufferSize, (void **) &inFileTable);
    if(inFileTable == NULL) {
        SysTbl -> BootServices -> FreePool(font -> glyphBuffer);
        SysTbl -> BootServices -> FreePool(font -> header);
        SysTbl -> BootServices -> FreePool(font);
        return NULL;
    }

    SysTbl -> BootServices -> AllocatePool(EfiLoaderData, 2 * 0xFFFF, (void **) &font -> unicodeTable);
    if(font -> unicodeTable == NULL) {
        SysTbl -> BootServices -> FreePool(inFileTable);
        SysTbl -> BootServices -> FreePool(font -> glyphBuffer);
        SysTbl -> BootServices -> FreePool(font -> header);
        SysTbl -> BootServices -> FreePool(font);
        return NULL;
    }

    file -> SetPosition(file, sizeof(PSFHeader) + glyphBufferSize);
    if(file -> Read(file, &fileSize, inFileTable) != EFI_SUCCESS) {
        SysTbl -> BootServices -> FreePool(inFileTable);
        SysTbl -> BootServices -> FreePool(font -> unicodeTable);
        SysTbl -> BootServices -> FreePool(font -> glyphBuffer);
        SysTbl -> BootServices -> FreePool(font -> header);
        SysTbl -> BootServices -> FreePool(font);
        return NULL;
    }

    u16 tblIdx = 0;
    u16 glyphIdx = 0;
    while(glyphIdx < glyphCount) {
        if(inFileTable[tblIdx] == 0xFFFF) {
            glyphIdx++;
            tblIdx++;
            continue;
        }

        font -> unicodeTable[inFileTable[tblIdx]] = glyphIdx;
        tblIdx++;
    }

    SysTbl -> BootServices -> FreePool(inFileTable);
    return font;
}