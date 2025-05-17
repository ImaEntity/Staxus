#ifndef HH_BEX
#define HH_BEX

#define BEX_SIGNATURE "BE\x04\xD5"
#define BEX_VERSION 0x0102 // Version 1.2

#define BEX_HEADER_LIBRAY        0b10000000
#define BEX_HEADER_REQUIRED_BASE 0b01000000

#define BEX_SEGMENT_EXECUTABLE 0b10000000
#define BEX_SEGMENT_WRITEABLE  0b01000000

#define BEX_RELOCATION_ABSOLUTE 0b10000000
#define BEX_RELOCATION_EXTERNAL 0b01000000

#define FlipEndian16(x) ( \
    ((x >> 8) & 0x00FF) | \
    ((x << 8) & 0xFF00) \
)

#define FlipEndian32(x) ( \
    ((x >> 24) & 0x000000FF) | \
    ((x >> 8)  & 0x0000FF00) | \
    ((x << 8)  & 0x00FF0000) | \
    ((x << 24) & 0xFF000000) \
)

#define FlipEndian64(x) ( \
    ((x >> 56) & 0x00000000000000FF) | \
    ((x >> 40) & 0x000000000000FF00) | \
    ((x >> 24) & 0x0000000000FF0000) | \
    ((x >> 8)  & 0x00000000FF000000) | \
    ((x << 8)  & 0x000000FF00000000) | \
    ((x << 24) & 0x0000FF0000000000) | \
    ((x << 40) & 0x00FF000000000000) | \
    ((x << 56) & 0xFF00000000000000) \
)

#include "types.h"

typedef struct {
    char signature[4];
    word version;
    byte flags;
    byte segmentCount;
    qword entryPoint;
    qword baseAddress;
} BEFileHeader;

typedef struct {
    char name[8];
    qword fileOffset;
    qword size;
    byte flags;
    byte reserved[7];
} BESegmentHeader;

typedef struct {
    qword offset;
    qword symNameOffset;
    word symNameLen;
    byte segmentIdx;
    byte size;
    byte flags;
    byte reserved[3];
} BERelocationEntry;

#endif