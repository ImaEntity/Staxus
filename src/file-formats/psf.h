#ifndef HH_FORMAT_PSF
#define HH_FORMAT_PSF

#include <types.h>
#include <gnu-efi/inc/efi.h>

#define PSF_MODE512    0x01
#define PSF_MODEHASTAB 0x02
#define PSF_MODESEQ    0x04

#define PSF_MAGIC 0x0436
typedef struct {
    u16 magic;
    u8  fontMode;
    u8  charSize;
} PSFHeader;

// i might be fkn stupid
// i think this is the v2 header

// #define PSF_FONT_MAGIC 0x864ab572
// typedef struct {
//     u32 magic;
//     u32 version;
//     u32 headerSize;
//     u32 flags;
//     u32 glyphCount;
//     u32 bytesPerGlyph;
//     u32 height;
//     u32 width;
// } PSFFontHeader;

typedef struct {
    PSFHeader     *header;
    byte          *glyphBuffer;
    u16           *unicodeTable;
} PSFFont;

PSFFont *LoadFontEFI(EFI_SYSTEM_TABLE *SysTbl, EFI_FILE *file, u64 fileSize);
PSFFont *LoadFont(const String path);

#endif