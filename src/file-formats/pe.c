#include "pe.h"
#include <types.h>
#include <memory/alloc.h>
#include <memory/utils.h>
#include <string/utils.h>
#include <file-systems/file-system.h>

struct __PEModule {
	String dir; 
	byte *image;
	PEHeader header;
	PE64Optional optional;
	PELibrary *dllRefs;
	u16 nDllRefs;
};

typedef struct __CachedFunc *CachedFunc;
struct __CachedFunc {
	String name;
	qword hash;
	u64 rva;
	CachedFunc next;
	u16 ordinal;
};

struct __PELibrary {
	qword hash;
	CachedFunc funcBuckets[64];
	PELibrary next;
	PEModule module;
	u16 refCount;
};

void (*GetPEFunctionAddress(PELibrary dll, const String name))() {
	qword funcHash = memhsh(name, strlen(name));
	CachedFunc func = dll -> funcBuckets[funcHash & 0x3F];

	while(func != NULL) {
		if(func -> hash == funcHash) break;
		func = func -> next;
	}

	if(func == NULL) return NULL;
	return (void (*)()) (u64) (dll -> module -> image + func -> rva); 
}

static PELibrary dllBuckets[256] = {0};
PELibrary LoadPELibrary(const String dir, const String dll) {
	u64 dlen = strlen(dir);
	String path = malloc(dlen + strlen(dll) + 2);
	if(path == NULL) return NULL;

	strcpy(path, dir);
	if(path[dlen - 1] != '/') strcat(path, "/");
	strcat(path, dll);

	FILE *fp = fopen(path, "rb");
	if(fp == NULL) {
		free(path);
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	u64 size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	byte *data = malloc(size);
	if(data == NULL) {
		fclose(fp);
		free(path);
		return NULL;
	}

	fread(data, 1, size, fp);
	fclose(fp);
	free(path);

	PEModule dllModule = LoadPEFile(dir, dll); free(data);
	if(dllModule == NULL) return NULL;
	
	PELibrary dllCache = malloc(sizeof(struct __PELibrary));
	if(dllCache == NULL) {
		UnloadPEFile(dllModule);
		return NULL;
	}

	qword hash = strihsh(dll);
	dllCache -> module = dllModule;
	dllCache -> refCount = 0;
	dllCache -> hash = hash;
	dllCache -> next = NULL;
	
	memset(dllCache -> funcBuckets, 0, sizeof(dllCache -> funcBuckets));

	PEDataDirectory export = dllModule -> optional.DataDirectory[PE_DIRECTORY_ENTRY_EXPORT];
	if(export.VirtualAddress == 0 || export.Size == 0) {
		free(dllCache);
		UnloadPEFile(dllModule);
		return NULL;
	}

	PEExportTable *table = (PEExportTable *) (dllModule -> image + export.VirtualAddress);
	u32 *functions = (u32 *) (dllModule -> image + table -> AddressTableRVA);
	u16 *ordinals = (u16 *) (dllModule -> image + table -> NameOrdinalTableRVA);
	u32 *names = (u32 *) (dllModule -> image + table -> NamePointerRVA);

	for(u32 i = 0; i < table -> NamePointerEntryCount; i++) {
		if(names[i] == 0) continue;

		CachedFunc func = malloc(sizeof(struct __CachedFunc));
		if(func == NULL) continue;

		func -> name = (String) (dllModule -> image + names[i]);
		func -> rva = functions[ordinals[i]];
		func -> ordinal = ordinals[i];
		func -> next = NULL;

		qword funcHash = memhsh(func -> name, strlen(func -> name));
		func -> hash = funcHash;

		if(dllCache -> funcBuckets[funcHash & 0x3F] != NULL) {
			CachedFunc p = dllCache -> funcBuckets[funcHash & 0x3F];
			while(p -> next != NULL) p = p -> next;
			p -> next = func;
		} else dllCache -> funcBuckets[funcHash & 0x3F] = func;
	}

	if(dllBuckets[hash & 0xFF] == NULL) {
		dllBuckets[hash & 0xFF] = dllCache;
		return dllCache;
	}

	PELibrary p = dllBuckets[hash & 0xFF];
	while(p -> next != NULL) p = p -> next;
	p -> next = dllCache;

	return dllCache;
}

void UnloadPELibrary(PELibrary dll) {
	PELibrary p = dllBuckets[dll -> hash & 0xFF];
	if(p -> hash != dll -> hash) {
		while(p -> next != NULL) {
			if(p -> next -> hash == dll -> hash) break;
			p = p -> next;
		}

		if(p -> next == NULL) return;
		p -> next = dll -> next;
	} else dllBuckets[dll -> hash & 0xFF] = dll -> next;

	for(u8 i = 0; i < 64; i++) {
		if(dll -> funcBuckets[i] == NULL) continue;
		while(dll -> funcBuckets[i] -> next != NULL) {
			CachedFunc current = dll -> funcBuckets[i];
			dll -> funcBuckets[i] = dll -> funcBuckets[i] -> next;
			free(current);
		}

		free(dll -> funcBuckets[i]);
	}

	UnloadPEFile(dll -> module);
	free(dll);
}

static boolean ResolvePEImports(PEModule module) {
	PEDataDirectory import = module -> optional.DataDirectory[PE_DIRECTORY_ENTRY_IMPORT];
	if(import.VirtualAddress == 0 || import.Size == 0) return true;

	PEImportTable *dlls = (PEImportTable *) (module -> image + import.VirtualAddress);
	for(u64 i = 0; dlls[i].Name != 0; i++) {
		PEImportTable *dll = &dlls[i];
		String dllName = (String) (module -> image + dll -> Name);
		qword dllHash = strihsh(dllName);

		PELibrary cachedDll = dllBuckets[dllHash & 0xFF];
		while(cachedDll != NULL) {
			if(cachedDll -> hash == dllHash) break;
			cachedDll = cachedDll -> next;
		}

		if(cachedDll == NULL) {
			if(module -> dir == NULL) return false;
			cachedDll = LoadPELibrary(module -> dir, dllName);
			if(cachedDll == NULL) return false;
		}

		cachedDll -> refCount++;
		module -> nDllRefs++;

		PELibrary *newRefs = realloc(module -> dllRefs, sizeof(PELibrary) * module -> nDllRefs);
		if(newRefs == NULL) {
			module -> nDllRefs--;
			cachedDll -> refCount--;

			if(cachedDll -> refCount == 0) UnloadPELibrary(cachedDll);
			return false;
		}

		module -> dllRefs = newRefs;

		if(module -> dllRefs == NULL) module -> nDllRefs--;
		else module -> dllRefs[module -> nDllRefs - 1] = cachedDll;

		u32 iatThunk = dll -> FirstThunk; 
		u32 lookupThunk = dll -> OriginalFirstThunk;
		if(lookupThunk == 0) lookupThunk = iatThunk;

		u32 *iat = (u32 *) (module -> image + iatThunk);
		u32 *lookup = (u32 *) (module -> image + lookupThunk);

		while(true) {
			u64 curLookup = *lookup++;
			String name = NULL;
			if(module -> optional.Magic == PE64_SIG) {
				curLookup |= (u64) *lookup++ << 32;
				if((curLookup & 0x8000000000000000) == 0)
					name = (String) (module -> image + curLookup + sizeof(word));
			} else if((curLookup & 0x80000000) == 0)
				name = (String) (module -> image + curLookup + sizeof(word));
			
			if(curLookup == 0) break;
			if(name == NULL) return false; // TODO: impl ordinals

			qword funcHash = memhsh(name, strlen(name));
			CachedFunc func = cachedDll -> funcBuckets[funcHash & 0x3F];

			while(func != NULL) {
				if(func -> hash == funcHash) break;
				func = func -> next;
			}

			if(func == NULL) return false;
			if(module -> optional.Magic == PE64_SIG) {
				u64 addr = (u64) cachedDll -> module -> image + func -> rva;
				*iat++ = addr & 0xFFFFFFFF; *iat++ = addr >> 32;
			} else *iat++ = (u32) (u64) cachedDll -> module -> image + func -> rva;
		}
	}

	return true;
}

static boolean ResolvePERelocations(PEModule module) {
	PEDataDirectory reloc = module -> optional.DataDirectory[PE_DIRECTORY_ENTRY_BASERELOC];
	if(reloc.VirtualAddress == 0 || reloc.Size == 0) return true;

	i64 delta = (i64) module -> image - module -> optional.ImageBase;
	if(delta == 0) return true;

	byte *cur = module -> image + reloc.VirtualAddress;
	byte *end = cur + reloc.Size;
	while(cur < end) {
		PERelocationTable *block = (PERelocationTable *) cur;
		if(block -> BlockSize < sizeof(PERelocationTable)) return false;

		u32 count = (block -> BlockSize - sizeof(PERelocationTable)) / sizeof(u16);
		u16 *entries = (u16 *) (cur + sizeof(PERelocationTable));

		for(u32 i = 0; i < count; i++) {
			u16 entry = entries[i];
			u16 type = entry >> 12;
			u16 off = entry & 0xFFF;

			switch(type) {
				default: return false;
				case PE_RELOC_ABS: break;

				case PE_RELOC_REL64:
					*(u64 *) (module -> image + block -> PageRVA + off) += delta;
					break;
				
				case PE_RELOC_REL32:
					*(u32 *) (module -> image + block -> PageRVA + off) += (u32) delta;
					break;
			}
		}

		cur += block -> BlockSize;
	}

	return true;
}

PEModule LoadPEFile(const String dir, const String name) {
	u64 dlen = strlen(dir);
	String path = malloc(dlen + strlen(name) + 2);
	if(path == NULL) return NULL;

	strcpy(path, dir);
	if(path[dlen - 1] != '/') strcat(path, "/");
	strcat(path, name);

	FILE *fp = fopen(path, "rb");
	if(fp == NULL) {
		free(path);
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	u64 size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	byte *data = malloc(size);
	if(data == NULL) {
		fclose(fp);
		free(path);
		return NULL;
	}

	fread(data, 1, size, fp);
	fclose(fp);
	free(path);

	if(size < 0x3C + 4) return NULL;
	if(*(u16 *) data != DOS_MAGIC) return NULL;

	u64 ptr = *(u32 *) (data + 0x3C);
	PEHeader *header = (PEHeader *) (data + ptr);
	if(header -> Magic != PE_MAGIC) return NULL;
	if(header -> SizeOfOptionalHeader == 0) return NULL;
	ptr += sizeof(PEHeader);

	PE64Optional optional = *(PE64Optional *) (header + 1);
	if(optional.Magic == PE32_SIG) {
		PE32Optional *realOpt = (PE32Optional *) (header + 1);

		// long enough runs of values that are the same
		memcpy(&optional.SectionAlignment, &realOpt -> SectionAlignment, 40);
		memcpy(optional.DataDirectory, realOpt -> DataDirectory, sizeof(optional.DataDirectory));
		
		optional.NumberOfRvaAndSizes = realOpt -> NumberOfRvaAndSizes;
		optional.SizeOfStackReserve = realOpt -> SizeOfStackReserve;
		optional.SizeOfStackCommit = realOpt -> SizeOfStackCommit;
		optional.SizeOfHeapReserve = realOpt -> SizeOfHeapReserve;
		optional.SizeOfHeapCommit = realOpt -> SizeOfHeapCommit;
		optional.LoaderFlags = realOpt -> LoaderFlags;
		optional.ImageBase = realOpt -> ImageBase;

		ptr += sizeof(PE32Optional);
	} else if(optional.Magic != PE64_SIG) return NULL;
	else ptr += sizeof(PE64Optional);

	PEModule module = malloc(sizeof(struct __PEModule));
	if(module == NULL) return NULL;

	module -> dir = dir;
	module -> header = *header;
	module -> optional = optional;
	module -> dllRefs = NULL;
	module -> nDllRefs = 0;

	module -> image = malloc(optional.SizeOfImage);
	if(module -> image == NULL) {
		free(module);
		return NULL;
	}

	memset(module -> image, 0, optional.SizeOfImage);
	for(u16 i = 0; i < header -> NumberOfSections; i++) {
		PESectionHeader *section = (PESectionHeader *) (data + ptr);

		memcpy(
			module -> image + section -> VirtualAddress,
			data + section -> PointerToRawData,
			section -> SizeOfRawData
		);

		ptr += sizeof(PESectionHeader);
	}

	memcpy(module -> image, data, ptr);
	free(data);

	if(!ResolvePEImports(module)) {
		UnloadPEFile(module);
		return NULL;
	}

	if(!ResolvePERelocations(module)) {
		UnloadPEFile(module);
		return NULL;
	}

	return module;
}

void (*GetPEEntry(PEModule module))(void) {
	if(module -> optional.AddressOfEntryPoint == 0) return NULL;
	return (void (*)(void)) (u64) (module -> image + module -> optional.AddressOfEntryPoint);
}

void UnloadPEFile(PEModule module) {
	if(module == NULL) return;
	if(module -> image != NULL) free(module -> image);

	if(module -> dllRefs != NULL) {
		for(u16 i = 0; i < module -> nDllRefs; i++) {
			module -> dllRefs[i] -> refCount--;
			if(module -> dllRefs[i] -> refCount == 0)
				UnloadPELibrary(module -> dllRefs[i]);
		}

		free(module -> dllRefs);
	}

	free(module);
}