#include "../gnu-efi/inc/efi.h"
#include "../gnu-efi/inc/efilib.h"

#include "../types.h"
#include "psf.h"

PSFFont *LoadFont(EFI_SYSTEM_TABLE *SysTbl, EFI_FILE *file) {
    PSFFont *font;
    // u64 infoSize = 0;
    // EFI_FILE_INFO *fileInfo;

    // file -> GetInfo(file, &gEfiFileInfoGuid, &infoSize, NULL);
    // SysTbl -> BootServices -> AllocatePool(EfiLoaderData, infoSize, (void **) &fileInfo);

    // file -> GetInfo(file, &gEfiFileInfoGuid, &infoSize, fileInfo);
    // u64 fileSize = fileInfo -> FileSize;

    // SysTbl -> BootServices -> FreePool(fileInfo);

    SysTbl -> BootServices -> AllocatePool(EfiLoaderData, sizeof(PSFFont), (void **) &font);
    if(font == NULL) return NULL;

    SysTbl -> BootServices -> AllocatePool(EfiLoaderData, sizeof(PSFHeader), (void **) &font -> header);
    if(font -> header == NULL) {
        SysTbl -> BootServices -> FreePool(font);
        return NULL;
    }
    
    // SysTbl -> BootServices -> AllocatePool(EfiLoaderData, sizeof(PSFFontHeader), (void **) &font -> fontHeader);
    // if(font -> fontHeader == NULL) {
    //     SysTbl -> BootServices -> FreePool(font -> header);
    //     SysTbl -> BootServices -> FreePool(font);
    //     return NULL;
    // }

    u64 headerSize = sizeof(PSFHeader);
    if(file -> Read(file, &headerSize, font -> header) != EFI_SUCCESS) {
        // SysTbl -> BootServices -> FreePool(font -> fontHeader);
        SysTbl -> BootServices -> FreePool(font -> header);
        SysTbl -> BootServices -> FreePool(font);
        return NULL;
    }

    // u64 fontHeaderSize = sizeof(PSFFontHeader);
    // if(file -> Read(file, &fontHeaderSize, font -> fontHeader) != EFI_SUCCESS) {
    //     SysTbl -> BootServices -> FreePool(font -> fontHeader);
    //     SysTbl -> BootServices -> FreePool(font -> header);
    //     SysTbl -> BootServices -> FreePool(font);
    //     return NULL;
    // }

    u64 glyphBufferSize = font -> header -> charSize * 256;
    if(font -> header -> fontMode & PSF_MODE512) glyphBufferSize *= 2;

    SysTbl -> BootServices -> AllocatePool(EfiLoaderData, glyphBufferSize, (void **) &font -> glyphBuffer);
    if(font -> glyphBuffer == NULL) {
        // SysTbl -> BootServices -> FreePool(font -> fontHeader);
        SysTbl -> BootServices -> FreePool(font -> header);
        SysTbl -> BootServices -> FreePool(font);
        return NULL;
    }

    file -> SetPosition(file, sizeof(PSFHeader));
    if(file -> Read(file, &glyphBufferSize, font -> glyphBuffer)) {
        SysTbl -> BootServices -> FreePool(font -> glyphBuffer);
        // SysTbl -> BootServices -> FreePool(font -> fontHeader);
        SysTbl -> BootServices -> FreePool(font -> header);
        SysTbl -> BootServices -> FreePool(font);
        return NULL;
    }

    if(font -> header -> magic != PSF_MAGIC) {
        SysTbl -> BootServices -> FreePool(font -> glyphBuffer);
        // SysTbl -> BootServices -> FreePool(font -> fontHeader);
        SysTbl -> BootServices -> FreePool(font -> header);
        SysTbl -> BootServices -> FreePool(font);
        return NULL;
    }

    return font;

    // // no unicode table
    // if(!font -> fontHeader -> flags)
    //     return font;
    
    // // build unicode table
    // byte *unicodeEntries;
    // u64 unicodeEntriesSize = fileSize - (
    //     sizeof(PSFHeader) + sizeof(PSFFontHeader) +
    //     font -> fontHeader -> glyphCount * font -> fontHeader -> bytesPerGlyph
    // );

    // SysTbl -> BootServices -> AllocatePool(EfiLoaderData, unicodeEntriesSize, (void **) &unicodeEntries);
    // if(font -> unicodeTable == NULL) {
    //     SysTbl -> BootServices -> FreePool(font -> glyphBuffer);
    //     SysTbl -> BootServices -> FreePool(font -> fontHeader);
    //     SysTbl -> BootServices -> FreePool(font -> header);
    //     SysTbl -> BootServices -> FreePool(font);
    //     return NULL;
    // }

    // if(file -> Read(file, &unicodeEntriesSize, unicodeEntries) != EFI_SUCCESS) {
    //     SysTbl -> BootServices -> FreePool(unicodeEntries);
    //     SysTbl -> BootServices -> FreePool(font -> glyphBuffer);
    //     SysTbl -> BootServices -> FreePool(font -> fontHeader);
    //     SysTbl -> BootServices -> FreePool(font -> header);
    //     SysTbl -> BootServices -> FreePool(font);
    //     return NULL;
    // }

    // SysTbl -> BootServices -> AllocatePool(EfiLoaderData, 0xFFFF * sizeof(u16), (void **) &font -> unicodeTable);
    // if(font -> unicodeTable == NULL) {
    //     SysTbl -> BootServices -> FreePool(unicodeEntries);
    //     SysTbl -> BootServices -> FreePool(font -> glyphBuffer);
    //     SysTbl -> BootServices -> FreePool(font -> fontHeader);
    //     SysTbl -> BootServices -> FreePool(font -> header);
    //     SysTbl -> BootServices -> FreePool(font);
    //     return NULL;
    // }

    // u16 glyph = 0;
    // u64 entryIdx = 0;
    // while(entryIdx < unicodeEntriesSize) {
    //     byte tblIdx = unicodeEntries[entryIdx];
    //     if(tblIdx == 0xFF) {
    //         glyph++;
    //         entryIdx++;
    //         continue;
    //     }

    //     if(tblIdx & 0x80 != 0) {
    //         if(tblIdx & 0x20 == 0) {
    //             tblIdx =
    //                 ((unicodeEntries[entryIdx] & 0x1F) << 6) +
    //                 (unicodeEntries[entryIdx + 1] & 0x3F);

    //             entryIdx++;
    //         } else if(tblIdx & 0x10 == 0) {
    //             tblIdx = ((
    //                 ((unicodeEntries[entryIdx] & 0xF) << 6) +
    //                 (unicodeEntries[entryIdx + 1] & 0x3F)
    //             ) << 6) + (unicodeEntries[entryIdx + 2] & 0x3F);

    //             entryIdx += 2;
    //         } else if(tblIdx & 0x08 == 0) {
    //             tblIdx = ((
    //                 ((((unicodeEntries[entryIdx] & 0x7) << 6) + (unicodeEntries[entryIdx + 1] & 0x3F)) << 6) +
    //                 (unicodeEntries[entryIdx + 2] & 0x3F)) << 6) + (unicodeEntries[entryIdx + 3] & 0x3F);
                
    //             entryIdx += 3;
    //         } else tblIdx = 0;
    //     }

    //     font -> unicodeTable[tblIdx] = glyph;
    //     entryIdx++;
    // }
    
    // SysTbl -> BootServices -> FreePool(unicodeEntries);
    // return font;
}