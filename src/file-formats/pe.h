#ifndef HH_FORMAT_PE
#define HH_FORMAT_PE

#include <types.h>

#define DOS_MAGIC 0x5A4D
#define PE_MAGIC 0x00004550
#define PE32_SIG 0x010B
#define PE64_SIG 0x020B

#define PE_DIRECTORY_ENTRY_EXPORT         0
#define PE_DIRECTORY_ENTRY_IMPORT         1
#define PE_DIRECTORY_ENTRY_RESOURCE       2
#define PE_DIRECTORY_ENTRY_EXCEPTION      3
#define PE_DIRECTORY_ENTRY_SECURITY       4
#define PE_DIRECTORY_ENTRY_BASERELOC      5
#define PE_DIRECTORY_ENTRY_DEBUG          6
#define PE_DIRECTORY_ENTRY_ARCHITECTURE   7
#define PE_DIRECTORY_ENTRY_GLOBALPTR      8
#define PE_DIRECTORY_ENTRY_TLS            9
#define PE_DIRECTORY_ENTRY_LOAD_CONFIG    10
#define PE_DIRECTORY_ENTRY_BOUND_IMPORT   11
#define PE_DIRECTORY_ENTRY_IAT            12
#define PE_DIRECTORY_ENTRY_DELAY_IMPORT   13
#define PE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14

#define PE_FILE_DLL 0x2000

#define PE_RELOC_ABS 0
#define PE_RELOC_REL64 10
#define PE_RELOC_REL32HIGH 1
#define PE_RELOC_REL32LOW 2
#define PE_RELOC_REL32 3

typedef struct {
	u32 Magic;
	u16 Machine;
	u16 NumberOfSections;
	u32 TimeDateStamp;
	u32 PointerToSymbolTable;
	u32 NumberOfSymbols;
	u16 SizeOfOptionalHeader;
	u16 Characteristics;
} PEHeader;

typedef struct {
	u32 VirtualAddress;
	u32 Size;
} PEDataDirectory;

typedef struct {
	u16 Magic; // 0x010B
	u8  MajorLinkerVersion;
	u8  MinorLinkerVersion;
	u32 SizeOfCode;
	u32 SizeOfInitializedData;
	u32 SizeOfUninitializedData;
	u32 AddressOfEntryPoint;
	u32 BaseOfCode;
	u32 BaseOfData;
	u32 ImageBase;
	u32 SectionAlignment;
	u32 FileAlignment;
	u16 MajorOperatingSystemVersion;
	u16 MinorOperatingSystemVersion;
	u16 MajorImageVersion;
	u16 MinorImageVersion;
	u16 MajorSubsystemVersion;
	u16 MinorSubsystemVersion;
	u32 Win32VersionValue;
	u32 SizeOfImage;
	u32 SizeOfHeaders;
	u32 CheckSum;
	u16 Subsystem;
	u16 DllCharacteristics;
	u32 SizeOfStackReserve;
	u32 SizeOfStackCommit;
	u32 SizeOfHeapReserve;
	u32 SizeOfHeapCommit;
	u32 LoaderFlags;
	u32 NumberOfRvaAndSizes;
	PEDataDirectory DataDirectory[16];
} PE32Optional;

typedef struct {
	u16 Magic; // 0x020B
	u8  MajorLinkerVersion;
	u8  MinorLinkerVersion;
	u32 SizeOfCode;
	u32 SizeOfInitializedData;
	u32 SizeOfUninitializedData;
	u32 AddressOfEntryPoint;
	u32 BaseOfCode;
	u64 ImageBase;
	u32 SectionAlignment;
	u32 FileAlignment;
	u16 MajorOperatingSystemVersion;
	u16 MinorOperatingSystemVersion;
	u16 MajorImageVersion;
	u16 MinorImageVersion;
	u16 MajorSubsystemVersion;
	u16 MinorSubsystemVersion;
	u32 Win32VersionValue;
	u32 SizeOfImage;
	u32 SizeOfHeaders;
	u32 CheckSum;
	u16 Subsystem;
	u16 DllCharacteristics;
	u64 SizeOfStackReserve;
	u64 SizeOfStackCommit;
	u64 SizeOfHeapReserve;
	u64 SizeOfHeapCommit;
	u32 LoaderFlags;
	u32 NumberOfRvaAndSizes;
	PEDataDirectory DataDirectory[16];
} PE64Optional;

typedef struct {
    char Name[8];
    u32  VirtualSize;
    u32  VirtualAddress;
    u32  SizeOfRawData;
    u32  PointerToRawData;
    u32  PointerToRelocations;
    u32  PointerToLinenumbers;
    u16  NumberOfRelocations;
    u16  NumberOfLinenumbers;
    u32  Characteristics;
} PESectionHeader;

typedef struct {
	u32 OriginalFirstThunk;
	u32 TimeDateStamp;
	u32 ForwarderChain;
	u32 Name;
	u32 FirstThunk;
} PEImportTable;

typedef struct {
	u32 ExportFlags;
	u32 TimeDateStamp;
	u16 MajorVersion;
	u16 MinorVersion;
	u32 NameRVA;
	u32 OrdinalBase;
	u32 AddressTableEntryCount;
	u32 NamePointerEntryCount;
	u32 AddressTableRVA;
	u32 NamePointerRVA;
	u32 NameOrdinalTableRVA;
} PEExportTable;

typedef struct {
	u32 PageRVA;
	u32 BlockSize;
} PERelocationTable;

typedef struct __PEModule *PEModule;
typedef struct __PELibrary *PELibrary;

PEModule LoadPEFile(const String dir, const String name);
void (*GetPEEntry(PEModule module))();
void UnloadPEFile(PEModule module);

PELibrary LoadPELibrary(const String dir, const String dll);
void (*GetPEFunctionAddress(PELibrary dll, const String name))();
void UnloadPELibrary(PELibrary dll);

#endif