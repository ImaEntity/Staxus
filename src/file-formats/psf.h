#ifndef HH_PSF
#define HH_PSF

#include "../types.h"
#include "../gnu-efi/inc/efi.h"

#define PSF_MODE512    0b00000001
#define PSF_MODEHASTAB 0b00000010
#define PSF_MODESEQ    0b00000100

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

PSFFont *LoadFont(EFI_SYSTEM_TABLE *SysTbl, EFI_FILE *file, u64 fileSize);

#endif