#ifndef HH_FORMAT_ELF
#define HH_FORMAT_ELF

#include <types.h>

#define ELF_MAGIC 0x464C457F

typedef struct {
	u32 Magic;
	u8  Class;
	u8  Data;
	u8  Version;
	u8  OSABI;
	u8  ABIVersion;
	u8  Padding[7];
	u16 Type;
	u16 Machine;
	u32 Version2;
	u32 Entry;
	u32 ProgramHeaderOffset;
	u32 SectionHeaderOffset;
	u32 Flags;
	u16 HeaderSize;
	u16 ProgramHeaderEntrySize;
	u16 ProgramHeaderCount;
	u16 SectionHeaderEntrySize;
	u16 SectionHeaderCount;
	u16 SectionNameStringTable;
} Elf32Header;

typedef struct {
	u32 Magic;
	u8  Class;
	u8  Data;
	u8  Version;
	u8  OSABI;
	u8  ABIVersion;
	u8  Padding[7];
	u16 Type;
	u16 Machine;
	u32 Version2;
	u64 Entry;
	u64 ProgramHeaderOffset;
	u64 SectionHeaderOffset;
	u32 Flags;
	u16 HeaderSize;
	u16 ProgramHeaderEntrySize;
	u16 ProgramHeaderCount;
	u16 SectionHeaderEntrySize;
	u16 SectionHeaderCount;
	u16 SectionNameStringTable;
} Elf64Header;

typedef struct {
    u32 Type;
    u32 Offset;
    u32 VirtualAddress;
    u32 PhysicalAddress;
    u32 FileSize;
    u32 MemorySize;
    u32 Flags;
    u32 Alignment;
} ELF32ProgramHeader;

typedef struct {
	u32 Type;
	u32 Flags;
	u64 Offset;
	u64 VirtualAddress;
	u64 PhysicalAddress;
	u64 FileSize;
	u64 MemorySize;
	u64 Alignment;
} Elf64ProgramHeader;

typedef struct {
    u32 Name;
    u32 Type;
    u32 Flags;
    u32 Address;
    u32 Offset;
    u32 Size;
    u32 Link;
    u32 Info;
    u32 AddressAlignment;
    u32 EntrySize;
} ELF32SectionHeader;

typedef struct {
    u32 Name;
    u32 Type;
    u64 Flags;
    u64 Address;
    u64 Offset;
    u64 Size;
    u32 Link;
    u32 Info;
    u64 AddressAlignment;
    u64 EntrySize;
} ELF64SectionHeader;

#endif