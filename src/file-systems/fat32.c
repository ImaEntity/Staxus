#include "fat32.h"
#include "file-system.h"

#include <memory/alloc.h>
#include <memory/utils.h>
#include <string/utils.h>

#include <video/print.h>

#define ENTRY_SIZE 32
#define Entry(cluster, idx) ((u64) cluster << 32 | idx)
#define EntryCluster(entry) (entry >> 32)
#define EntryOffset(entry) ((entry & 0xFFFFFFFF) * ENTRY_SIZE)

#define ENDOFCHAIN (0x0FFFFFF8)
#define ATTRIBUTE_DIRECTORY 0x10
#define ATTRIBUTE_LONG_FILE_NAME 0x0F

typedef struct {
    BlockDevice *device;
    u32 fatStart;
    u32 dataStart;
    u32 rootCluster;
    u32 sectorsPerFAT;
    u64 bytesPerCluster;
    u8  sectorsPerCluster;
} FAT32Mount;

boolean probeFAT32FileSystem(BlockDevice *device) {
    u8 *buf = aalloc(device -> sectorSize, 128);
    if(buf == NULL) return false;

    if(!ReadBlockDevice(device, buf, 0, 1)) {
        free(buf);
        return false;
    }

    if(buf[510] != 0x55 || buf[511] != 0xAA) {
        free(buf);
        return false;
    }

    u32 fatSz32 = *(u32 *) (buf + 36);
    if(fatSz32 == 0) {
        free(buf);
        return false;
    }

    free(buf);
    return true;
}


void *mountFAT32FileSystem(BlockDevice *device, const String path) {
    FAT32Mount *mount = malloc(sizeof(FAT32Mount));
    if(mount == NULL) return NULL;

    byte *buf = aalloc(device -> sectorSize, 128);
    if(buf == NULL) {
        free(mount);
        return NULL;
    }

    if(!ReadBlockDevice(device, buf, 0, 1)) {
        free(buf);
        free(mount);

        return NULL;
    }

    u16 rsvd = *(u16 *) (buf + 14);
    byte fats = buf[16];
    u32 fatSz32 = *(u32 *) (buf + 36);

    mount -> device = device;
    mount -> fatStart = rsvd;
    mount -> dataStart = rsvd + fats * fatSz32;
    mount -> rootCluster = *(u32 *) (buf + 44);
    mount -> sectorsPerFAT = fatSz32;
    mount -> bytesPerCluster = buf[13] * device -> sectorSize;
    mount -> sectorsPerCluster = buf[13];

    return mount;
}

void unmountFAT32FileSystem(FSMount *mount) {
    FAT32Mount *fatMount = (FAT32Mount *) mount -> internal;
    free(fatMount);
}

boolean readCluster(FAT32Mount *mount, void *buffer, u32 cluster) {
    BlockDevice *device = mount -> device;
    u32 firstSector = mount -> dataStart + (cluster - 2) * mount -> sectorsPerCluster;
    return ReadBlockDevice(device, buffer, firstSector, mount -> sectorsPerCluster);
}

boolean writeCluster(FAT32Mount *mount, void *buffer, u32 cluster) {
    BlockDevice *device = mount -> device;
    u32 firstSector = mount -> dataStart + (cluster - 2) * mount -> sectorsPerCluster;
    return WriteBlockDevice(device, buffer, firstSector, mount -> sectorsPerCluster);
}

u32 getNextCluster(FAT32Mount *mount, u32 cluster) {
    BlockDevice *device = mount -> device;
    u32 fatOffset = cluster * 4;
    u32 fatSector = mount -> fatStart + (fatOffset / device -> sectorSize);
    u32 entryOffset = fatOffset % device -> sectorSize;

    u8 *buf = aalloc(device -> sectorSize, 128);
    if(buf == NULL) return ENDOFCHAIN;

    if(!ReadBlockDevice(device, buf, fatSector, 1)) {
        free(buf);
        return ENDOFCHAIN;
    }

    u32 next = *(u32 *) (buf + entryOffset);
    free(buf);

    return next & 0x0FFFFFFF;
}

u32 allocCluster(FAT32Mount *mount) {
    BlockDevice *dev = mount -> device;
    u32 totalClusters = dev -> sectorCount / mount -> sectorsPerCluster;

    byte *buf = aalloc(dev -> sectorSize * mount -> sectorsPerFAT, 128);
    if(buf == NULL) return ENDOFCHAIN;

    if(!ReadBlockDevice(dev, buf, mount -> fatStart, mount -> sectorsPerFAT)) {
        free(buf);
        return ENDOFCHAIN;
    }

    u32 maxFatEntries = (mount -> sectorsPerFAT * dev -> sectorSize) / 4;
    u32 loopLimit = totalClusters < maxFatEntries ? totalClusters : maxFatEntries;

    for(u32 i = 2; i < loopLimit; i++) {
        u32 fatOffset = i * 4;
        u32 value = *(u32 *) (buf + fatOffset);
        if((value & 0x0FFFFFFF) != 0x00000000) continue;
        
        *(u32 *) (buf + fatOffset) = ENDOFCHAIN;
        
        u32 fatSector = mount -> fatStart + (fatOffset / dev -> sectorSize);
        u32 sectorBufIndex = fatSector - mount -> fatStart;
        u32 bufOffset = sectorBufIndex * dev -> sectorSize;

        if(!WriteBlockDevice(dev, buf + bufOffset, fatSector, 1)) {
            free(buf);
            return ENDOFCHAIN;
        }

        u32 fatBackup = fatSector + mount -> sectorsPerFAT;
        if(!WriteBlockDevice(dev, buf + bufOffset, fatBackup, 1)) {
            free(buf);
            return ENDOFCHAIN;
        }

        free(buf);
        
        byte *clusterBuf = aalloc(mount -> bytesPerCluster, 128);
        if(clusterBuf == NULL) return ENDOFCHAIN;

        memset(clusterBuf, 0, mount -> bytesPerCluster);
        if(!writeCluster(mount, clusterBuf, i)) {
            free(clusterBuf);
            return ENDOFCHAIN;
        }

        free(clusterBuf);
        return i;
    }

    free(buf);
    return ENDOFCHAIN;
}

boolean setNextCluster(FAT32Mount *mount, u32 cluster, u32 value) {
    BlockDevice *device = mount -> device;
    u32 fatOffset = cluster * 4;
    u32 fatSector = mount -> fatStart + (fatOffset / device -> sectorSize);
    u32 entryOffset = fatOffset % device -> sectorSize;

    u32 rwSectors = (entryOffset > device -> sectorSize - 4) ? 2 : 1;
    u8 *buf = aalloc(device -> sectorSize * rwSectors, 128);
    if(buf == NULL) return false;

    if(!ReadBlockDevice(device, buf, fatSector, rwSectors)) {
        free(buf);
        return false;
    }

    u32 old = *(u32 *) (buf + entryOffset);
    *(u32 *) (buf + entryOffset) = (old & 0xF0000000) | (value & 0x0FFFFFFF);

    boolean result = WriteBlockDevice(device, buf, fatSector, rwSectors);

    u32 fatBackup = fatSector + mount -> sectorsPerFAT;
    result &= WriteBlockDevice(device, buf, fatBackup, rwSectors);

    free(buf);
    return result;
}

typedef struct {
    u32 parentCluster;
    u32 parentEntryIdx;
    
    String name;
    u32 cluster;
    byte attr;
    u32 size;

    byte crtTimeTenth;
    u16 crtTime;
    u16 crtDate;
    u16 accDate;
    u16 wrtTime;
    u16 wrtDate;
} FAT32Entry;

FAT32Entry *parseFAT32Entry(byte *entry, char *lfnBuf, boolean *lfn) {
    if(entry[11] == ATTRIBUTE_LONG_FILE_NAME) {
        int pos = 13 * ((entry[0] & 0x1F) - 1);

        for(int i = 0; i < 5; i++) lfnBuf[pos++] = entry[ 1 + i * 2];
        for(int i = 0; i < 6; i++) lfnBuf[pos++] = entry[14 + i * 2];
        for(int i = 0; i < 2; i++) lfnBuf[pos++] = entry[28 + i * 2];
        lfnBuf[pos] = 0;

        if((entry[0] & 0x40) != 0) *lfn = true;
        return NULL;
    }

    if(entry[0] == 0x00) return NULL; // end of dir
    if(entry[0] == 0xE5) return NULL; // unused / deleted

    u16 dataClusterH = entry[21] << 8 | entry[20];
    u16 dataClusterL = entry[27] << 8 | entry[26];
    u32 dataCluster = (u32) dataClusterH << 16 | dataClusterL;

    FAT32Entry *res = malloc(sizeof(FAT32Entry));
    if(res == NULL) return NULL;

    res -> crtTimeTenth = entry[17];
    res -> crtTime = (u16) entry[17] << 8 | entry[16];
    res -> crtDate = (u16) entry[19] << 8 | entry[18];
    res -> accDate = (u16) entry[23] << 8 | entry[22];
    res -> wrtTime = (u16) entry[25] << 8 | entry[24];
    res -> wrtDate = (u16) entry[27] << 8 | entry[26];

    res -> size    = *(u32 *) (entry + 28);
    res -> cluster = dataCluster;
    res -> attr    = entry[11];

    if(!*lfn) { // disgusting syntax
        char name[13];

        for(int i = 0; i < 8; i++) name[i] = entry[i];
        name[8] = 0;

        for(int i = 7; i >= 0; i--) {
            if(name[i] != ' ') break;
            name[i] = 0;
        }

        u8 fnLen = strlen(name);
        name[fnLen] = '.';

        for(int i = 0; i < 3; i++) name[fnLen + 1 + i] = entry[8 + i];
        name[fnLen + 4] = 0;

        for(int i = 2; i >= 0; i--) {
            if(name[fnLen + 1 + i] != ' ') break;
            name[fnLen + 1 + i] = 0;
        }

        u8 extLen = strlen(name + fnLen + 1);
        if(extLen == 0) name[fnLen] = 0;

        res -> name = malloc(strlen(name) + 1);
        if(res -> name == NULL) {
            free(res);
            return NULL;
        }

        strcpy(res -> name, name);
        return res;
    }

    res -> name = malloc(strlen(lfnBuf) + 1);
    if(res -> name == NULL) {
        free(res);
        return NULL;
    }

    strcpy(res -> name, lfnBuf);
    *lfn = false;

    return res;
}

u32 getRequiredLFNEntries(const String name) {
    if(name[0] == 0x00 || name[0] == 0xE5) return 0;

    u64 len = strlen(name);
    String dot = strchr(name, '.');    

    u64 fnlen = dot != NULL ? dot - name : len;
    u64 exlen = dot != NULL ? strlen(dot + 1) : 0;

    if(fnlen <= 8 && exlen <= 3 && strchr(name, ' ') == NULL && strchr(name, '+') == NULL) {
        boolean needsLFN = false;
        for(int i = 0; i < len; i++) {
            if(islower(name[i])) {
                needsLFN = true;
                break;
            }
        }

        if(!needsLFN) return 0;
    }

    u32 entries = (len + 12) / 13;
    if(entries > 20) return 20;

    return entries;
}

boolean check83NameExists(FAT32Mount *mount, u32 parentCluster, const String name) {
    byte *clusterBuf = aalloc(mount -> bytesPerCluster, 128);
    if(clusterBuf == NULL) return false;
    
    u32 cluster = parentCluster;
    while(cluster < ENDOFCHAIN) {
        if(!readCluster(mount, clusterBuf, cluster)) {
            free(clusterBuf);
            return false;
        }

        for(u32 eIdx = 0; eIdx < mount -> bytesPerCluster; eIdx += 32) {
            if(memcmp(name, clusterBuf + eIdx, 11) == 0)
                return true;
        }

        cluster = getNextCluster(mount, cluster);
    }

    return false;
}

boolean get83Name(FAT32Mount *mount, u32 parentCluster, const String name, String buf) {
    memset(buf, ' ', 11);
    
    if(name[0] == 0x00 || name[0] == 0xE5) {
        buf[0] = name[0];
        return true;
    }

    u64 len = strlen(name);
    String dot = strchr(name, '.');

    u64 fnlen = dot != NULL ? dot - name : len;
    u64 exlen = dot != NULL ? strlen(dot + 1) : 0;

    boolean fits = (fnlen <= 8 && exlen <= 3);
    for(u32 i = 0; i < fnlen && fits; i++) {
        if(isupper(name[i])) continue;
        if(strchr("0123456789$%'-_@~`!(){}^#&", name[i]) != NULL) continue;

        fits = false;
        break;
    }

    if(fits) {
        for(u32 i = 0; i < fnlen; i++)
            buf[i] = toupper(name[i]);

        if(dot != NULL) {
            for(int i = 0; i < exlen && i < 3; i++)
                buf[8 + i] = toupper(dot[1 + i]);
        }

        if(!check83NameExists(mount, parentCluster, buf))
            return true;
    }

    char nbuf[8];
    for(int n = 1; n < 1000000; n++) {
        memset(buf, ' ', 11);
        for(u32 i = 0; i < fnlen; i++)
            buf[i] = toupper(name[i]);

        sprintf(nbuf, "%d", n);
        u64 nlen = strlen(nbuf);

        u32 copyLen = fnlen >= 8 - nlen - 1 ? 8 - nlen - 1 : fnlen;
        memcpy(buf + copyLen + 1, nbuf, nlen);
        buf[copyLen] = '~';

        if(dot != NULL) {
            for(int i = 0; i < exlen && i < 3; i++)
                buf[8 + i] = toupper(dot[1 + i]);
        }

        if(!check83NameExists(mount, parentCluster, buf))
            return true;
    }

    return false;
}

byte checksum83Name(const String name) {
    u16 sum = 0;
    for(int i = 0; i < 11; i++) {
        sum = ((sum & 1) << 7) + (sum >> 1) + name[i];
        sum &= 0xFF;
    }

    return sum;
}

u32 findFreeEntries(FAT32Mount *mount, u32 cluster, u32 needed) {
    byte *buf = aalloc(mount -> bytesPerCluster, 128);
    if(buf == NULL) return ENDOFCHAIN;

    u32 entriesPerCluster = mount -> bytesPerCluster / ENTRY_SIZE;
    u32 start = ENDOFCHAIN;
    u32 clusters = 0;
    u32 found = 0;

    while(cluster < ENDOFCHAIN) {
        if(!readCluster(mount, buf, cluster)) {
            free(buf);
            return ENDOFCHAIN;
        }

        for(u64 eIdx = 0; eIdx < entriesPerCluster; eIdx++) {
            byte *raw = buf + eIdx * ENTRY_SIZE;
            if(*raw == 0xE5 || *raw == 0x00) {
                if(found == 0) start = clusters * entriesPerCluster + eIdx;
                found++;

                if(found == needed) {
                    free(buf);
                    return start;
                }
            } else {
                start = ENDOFCHAIN;
                found = 0; 
            }
        }

        cluster = getNextCluster(mount, cluster);
        clusters++;
    }

    free(buf);
    return ENDOFCHAIN;
}

boolean extendFAT32File(FAT32Mount *mount, u32 *startCluster, u32 existingClusters, u32 neededClusters) {
    if(neededClusters <= existingClusters) return true;
    
    u32 clustersAdd = neededClusters - existingClusters;
    u32 lastCluster = *startCluster;
    if(existingClusters > 0) {
        for(u32 i = 1; i < existingClusters; i++)
            lastCluster = getNextCluster(mount, lastCluster);
    }

    for(u32 i = 0; i < clustersAdd; i++) {
        u32 newCluster = allocCluster(mount);
        if(newCluster >= ENDOFCHAIN) return false;

        if(existingClusters == 0 && i == 0) *startCluster = newCluster;
        else setNextCluster(mount, lastCluster, newCluster);

        lastCluster = newCluster;
    }

    return setNextCluster(mount, lastCluster, ENDOFCHAIN);
}

boolean writeFAT32Entry(FAT32Mount *mount, FAT32Entry *entry, boolean inPlace) {
    u32 lfnEntries = !inPlace ? getRequiredLFNEntries(entry -> name) : 0;
    u32 startEntry = !inPlace ?
        findFreeEntries(mount, entry -> parentCluster, lfnEntries + 1) :
        entry -> parentEntryIdx;

    u32 entriesPerCluster = mount -> bytesPerCluster / ENTRY_SIZE;
    byte *clusterBuf = aalloc(mount -> bytesPerCluster, 128);
    if(clusterBuf == NULL) return false;

    if(startEntry >= ENDOFCHAIN) {
        u32 clusters = 1;
        u32 last = entry -> parentCluster;
        while(last < ENDOFCHAIN) {
            u32 next = getNextCluster(mount, last);
            if(next >= ENDOFCHAIN) break;
            last = next;
            clusters++;
        }

        if(!extendFAT32File(mount, &entry -> parentCluster, clusters, clusters + 1)) {
            free(clusterBuf);
            return false;
        }

        startEntry = clusters * entriesPerCluster;
    }
    
    entry -> parentEntryIdx = startEntry + lfnEntries;

    u32 clusterOff = entry -> parentEntryIdx * ENTRY_SIZE / mount -> bytesPerCluster;
    u32 parentCluster = entry -> parentCluster;
    for(u32 i = 0; i < clusterOff; i++)
        parentCluster = getNextCluster(mount, parentCluster);

    if(!readCluster(mount, clusterBuf, parentCluster)) {
        free(clusterBuf);
        return false;
    }

    char n83buf[11];
    if(!get83Name(mount, entry -> parentCluster, entry -> name, n83buf)) {
        free(clusterBuf);
        return false;
    }

    byte checksum = checksum83Name(n83buf);
    for(u32 i = 0; i < lfnEntries; i++) {
        byte eIdx = lfnEntries - 1 - i;

        byte *raw = clusterBuf + ((startEntry + eIdx) % entriesPerCluster) * ENTRY_SIZE;
        memset(raw, 0x00, 32);

        u8 seq = eIdx + 1;
        if(i == 0) seq |= 0x40;
        raw[0] = seq;
        
        raw[11] = 0x0F;

        raw[12] = 0x00;
        raw[13] = checksum;

        raw[26] = 0x00;
        raw[27] = 0x00;

        u64 startPos = eIdx * 13;
        u64 len = strlen(entry -> name) + 1;

        for(int i = 0; i < 5; i++) {
            int pos = startPos + i;
            word ch = pos < len ? entry -> name[pos] : 0xFFFF;

            raw[1 + 2 * i] = ch & 0xFF;
            raw[2 + 2 * i] = ch >> 8;
        }

        for(int i = 0; i < 6; i++) {
            int pos = startPos + i + 5;
            word ch = pos < len ? entry -> name[pos] : 0xFFFF;
            
            raw[14 + 2 * i] = ch & 0xFF;
            raw[15 + 2 * i] = ch >> 8;
        }

        for(int i = 0; i < 2; i++) {
            int pos = startPos + i + 11;
            word ch = pos < len ? entry -> name[pos] : 0xFFFF;
            
            raw[28 + 2 * i] = ch & 0xFF;
            raw[29 + 2 * i] = ch >> 8;
        }
    }

    byte *raw = clusterBuf + (entry -> parentEntryIdx % entriesPerCluster) * ENTRY_SIZE;
    byte first = entry -> name[0];

    if(!inPlace || first == 0x00 || first == 0xE5) memcpy(raw, n83buf, 11);
    raw[11] = entry -> attr;

    raw[26] = (entry -> cluster >>  0) & 0xFF;
    raw[27] = (entry -> cluster >>  8) & 0xFF;
    raw[20] = (entry -> cluster >> 16) & 0xFF;
    raw[21] = (entry -> cluster >> 24) & 0xFF;

    raw[28] = (entry -> size >>  0) & 0xFF;
    raw[29] = (entry -> size >>  8) & 0xFF;
    raw[30] = (entry -> size >> 16) & 0xFF;
    raw[31] = (entry -> size >> 24) & 0xFF;

    if(!writeCluster(mount, clusterBuf, parentCluster)) {
        free(clusterBuf);
        return false;
    }

    free(clusterBuf);
    return true;
}

void destroyFAT32Entry(FAT32Entry *entry) {
    if(entry == NULL) return;
    if(entry -> name != NULL) free(entry -> name);
    free(entry);
}

typedef struct {
    u32 startCluster;
    u32 cluster;
    u32 entryIdx;
    u32 localIdx;
    byte *clusterBuf;
    char lfnBuf[260];
    boolean lfn;
} FAT32Iterator;

FAT32Iterator *createFAT32ClusterIterator(FAT32Mount *mount, u32 startCluster) {
    FAT32Iterator *iter = malloc(sizeof(FAT32Iterator));
    if(iter == NULL) return NULL;

    iter -> startCluster = startCluster;
    iter -> cluster = startCluster;
    iter -> clusterBuf = NULL;
    iter -> entryIdx = 0;
    iter -> localIdx = 0;
    iter -> lfn = false;

    if(startCluster >= ENDOFCHAIN)
        return iter;

    iter -> clusterBuf = aalloc(mount -> bytesPerCluster, 128);
    if(iter -> clusterBuf == NULL) {
        free(iter);
        return NULL;
    }

    if(!readCluster(mount, iter -> clusterBuf, startCluster)) {
        free(iter -> clusterBuf);
        free(iter);
        return NULL;
    }

    return iter;
}

FAT32Entry *nextFAT32Entry(FAT32Mount *mount, FAT32Iterator *iter) {
    u32 entriesPerCluster = mount -> bytesPerCluster / ENTRY_SIZE;
    
    while(iter -> cluster < ENDOFCHAIN) {
        while(iter -> localIdx < entriesPerCluster) {
            byte *raw = iter -> clusterBuf + iter -> localIdx * ENTRY_SIZE;
            iter -> localIdx++;
            iter -> entryIdx++;

            FAT32Entry *entry = parseFAT32Entry(raw, iter -> lfnBuf, &iter -> lfn);
            if(entry == NULL) continue;

            entry -> parentCluster = iter -> startCluster;
            entry -> parentEntryIdx = iter -> entryIdx - 1;

            return entry;
        }

        iter -> cluster = getNextCluster(mount, iter -> cluster);
        iter -> localIdx = 0;

        if(
            iter -> cluster < ENDOFCHAIN &&
            !readCluster(mount, iter -> clusterBuf, iter -> cluster)
        ) return NULL;
    }

    return NULL;
}

void destroyFAT32Iterator(FAT32Iterator *iter) {
    if(iter == NULL) return;
    if(iter -> clusterBuf != NULL) free(iter -> clusterBuf);
    free(iter);
}

FAT32Entry *findFAT32Entry(FAT32Mount *mount, const String path, u32 startCluster) {
    FAT32Iterator *iter = createFAT32ClusterIterator(mount, startCluster);
    if(iter == NULL) return NULL;

    String rawName = malloc(strlen(path) + 1);
    if(rawName == NULL) {
        destroyFAT32Iterator(iter);
        return NULL;
    }

    int chrIdx = 0;

    strcpy(rawName, path);
    String name = rawName; 

    if(name[chrIdx] == '/') name++;
    if(name[chrIdx] == 0) {
        free(rawName);
        destroyFAT32Iterator(iter);

        FAT32Entry *root = malloc(sizeof(FAT32Entry));
        if(root == NULL) return NULL;

        root -> parentCluster = 0x00000001;
        root -> parentEntryIdx = 0;
    
        root -> cluster = startCluster;
        root -> attr = ATTRIBUTE_DIRECTORY;
        root -> size = 0;
        root -> name = malloc(1);
        if(root -> name == NULL) {
            free(root);
            return NULL;
        }

        root -> name[0] = 0;
        root -> crtTimeTenth = 0;
        root -> crtTime = 0;
        root -> crtDate = 0;
        root -> accDate = 0;
        root -> wrtTime = 0;
        root -> wrtDate = 0;

        return root;
    }

    while(name[chrIdx] != '/' && name[chrIdx] != 0) chrIdx++;
    name[chrIdx] = 0;

    FAT32Entry *entry;
    while((entry = nextFAT32Entry(mount, iter)) != NULL) {
        if(stricmp(entry -> name, name) != 0) {
            destroyFAT32Entry(entry);
            continue;
        }

        if(path[chrIdx] == 0) {
            free(rawName);
            destroyFAT32Iterator(iter);
            return entry;
        }

        if((entry -> attr & ATTRIBUTE_DIRECTORY) == 0) {
            destroyFAT32Entry(entry);
            destroyFAT32Iterator(iter);
            free(rawName);
            return NULL;
        }

        u32 cluster = entry -> cluster;

        destroyFAT32Entry(entry);
        destroyFAT32Iterator(iter);
        free(rawName);

        return findFAT32Entry(
            mount,
            path + chrIdx + 1,
            cluster
        );
    }

    destroyFAT32Iterator(iter);
    free(rawName);
    return NULL;
}

typedef struct {
    u32 size;
    u32 cluster;
    byte attr;
} FAT32EntryInfo;

FAT32Entry *createFAT32Entry(FAT32Mount *mount, const String path, FAT32EntryInfo info) {
    FAT32Entry *entry = findFAT32Entry(mount, path, mount -> rootCluster);
    if(entry != NULL) return entry;

    String dir = malloc(strlen(path) + 1);
    if(dir == NULL) return NULL;

    strcpy(dir, path);

    String fname = basepath(dir);
    String dname = dirname(dir);

    FAT32Entry *dirEntry = findFAT32Entry(mount, dname, mount -> rootCluster);
    if(dirEntry == NULL) {
        free(dir);
        return NULL;
    }

    FAT32Entry *newEntry = malloc(sizeof(FAT32Entry));
    if(newEntry == NULL) {
        free(dir);
        destroyFAT32Entry(dirEntry);
        return NULL;
    }

    newEntry -> parentCluster = dirEntry -> cluster;
    newEntry -> cluster = info.cluster >= ENDOFCHAIN ? allocCluster(mount) : info.cluster;
    newEntry -> attr = info.attr;
    newEntry -> size = info.size;

    if(info.cluster >= ENDOFCHAIN && newEntry -> cluster >= ENDOFCHAIN) {
        free(dir); free(newEntry);
        destroyFAT32Entry(dirEntry);
        return NULL;
    }

    if(info.cluster >= ENDOFCHAIN)
        setNextCluster(mount, newEntry -> cluster, ENDOFCHAIN);

    newEntry -> name = malloc(strlen(fname) + 1);
    if(newEntry -> name == NULL) {
        free(dir); free(newEntry);
        destroyFAT32Entry(dirEntry);
        return NULL;
    }

    strcpy(newEntry -> name, fname);

    byte *buf = aalloc(mount -> bytesPerCluster, 128);
    if(buf == NULL) {
        free(dir);
        destroyFAT32Entry(dirEntry);
        destroyFAT32Entry(newEntry);
        return NULL;
    }

    if(!writeFAT32Entry(mount, newEntry, false)) {
        free(buf); free(dir);
        destroyFAT32Entry(dirEntry);
        destroyFAT32Entry(newEntry);
        return NULL;
    }

    return newEntry;
}

FAT32Iterator *createFAT32PathIterator(FAT32Mount *mount, const String path) {
    FAT32Entry *entry = findFAT32Entry(mount, path, mount -> rootCluster);
    if(entry == NULL) return NULL;


    if((entry -> attr & ATTRIBUTE_DIRECTORY) == 0) {
        destroyFAT32Entry(entry);
        return NULL;
    }

    FAT32Iterator *iter = createFAT32ClusterIterator(mount, entry -> cluster);
    destroyFAT32Entry(entry);

    return iter;
}

boolean resetFAT32ClusterChain(FAT32Mount *mount, u32 startCluster) {
    u32 cluster = startCluster;
    while(cluster < ENDOFCHAIN) {
        u32 next = getNextCluster(mount, cluster);
        if(!setNextCluster(mount, cluster, 0x00000000)) return false;
        cluster = next;
    }

    return setNextCluster(mount, startCluster, ENDOFCHAIN);
}


FILE *openFAT32File(FSMount *mount, const String relPath, byte flags) {
    FAT32Mount *fatMount = (FAT32Mount *) mount -> internal;

    FAT32Entry *entry;
    if((flags & FILE_OPEN_CREATE) != 0) entry = createFAT32Entry(fatMount, relPath, (FAT32EntryInfo) {0, ENDOFCHAIN, 0});
    else entry = findFAT32Entry(fatMount, relPath, fatMount -> rootCluster);
    if(entry == NULL) return NULL;

    if((entry -> attr & ATTRIBUTE_DIRECTORY) != 0) {
        destroyFAT32Entry(entry);
        return NULL;
    }

    if((flags & FILE_OPEN_TRUNC) != 0) {
        entry -> size = 0;
        if(!resetFAT32ClusterChain(fatMount, entry -> cluster)) {
            destroyFAT32Entry(entry);
            return NULL;
        }
    }

    FILE *file = malloc(sizeof(FILE));
    if(file == NULL) {
        destroyFAT32Entry(entry);
        return NULL;
    }

    file -> size = entry -> size;
    file -> ptr = (flags & FILE_OPEN_APPEND) != 0 ? file -> size : 0;
    file -> internal = entry;
    file -> mount = mount;
    file -> flags = flags;

    return file;
}

u64 readFAT32File(FILE *file, void *buffer, u64 offset, u64 length) {
    FAT32Entry *entry = (FAT32Entry *) file -> internal;
    FAT32Mount *mount = (FAT32Mount *) file -> mount -> internal;

    if(offset + length > entry -> size)
        length = entry -> size - offset;

    u64 clusterOffset = offset / mount -> bytesPerCluster;
    u64 byteOffset = offset % mount -> bytesPerCluster;

    u32 currentCluster = entry -> cluster;
    for(u64 i = 0; i < clusterOffset; i++)
        currentCluster = getNextCluster(mount, currentCluster);

    u64 bytesLeft = length;
    u64 totalBytesRead = 0;

    byte *buf = aalloc(mount -> bytesPerCluster, 128);
    if(buf == NULL) return 0;

    while(bytesLeft > 0) {
        if(!readCluster(mount, buf, currentCluster)) {
            free(buf);
            return 0;
        }

        u64 toRead = bytesLeft > mount -> bytesPerCluster - byteOffset ?
            mount -> bytesPerCluster - byteOffset :
            bytesLeft;

        memcpy(buffer + totalBytesRead, buf + byteOffset, toRead);

        bytesLeft -= toRead;
        totalBytesRead += toRead;
        byteOffset = 0;

        currentCluster = getNextCluster(mount, currentCluster);
    }

    free(buf);
    return totalBytesRead;  
}

u64 writeFAT32File(FILE *file, void *buffer, u64 offset, u64 length) {
    FAT32Entry *entry = (FAT32Entry *) file -> internal;
    FAT32Mount *mount = (FAT32Mount *) file -> mount -> internal;

    u32 existingClusters = entry -> size != 0 ? (entry -> size + mount -> bytesPerCluster - 1) / mount -> bytesPerCluster : 1;
    u32 neededClusters = (length + offset + mount -> bytesPerCluster - 1) / mount -> bytesPerCluster;
    // printf("%d %d - ", existingClusters, neededClusters);

    if(offset + length > entry -> size) {
        if(!extendFAT32File(mount, &entry -> cluster, existingClusters, neededClusters))
            return 0;

        entry -> size = length + offset;
        writeFAT32Entry(mount, entry, true);
        
        file -> size = entry -> size;
    }

    u64 clusterOffset = offset / mount -> bytesPerCluster;
    u64 byteOffset = offset % mount -> bytesPerCluster;

    u32 currentCluster = entry -> cluster;
    for(u64 i = 0; i < clusterOffset; i++)
        currentCluster = getNextCluster(mount, currentCluster);

    u64 bytesLeft = length;
    u64 totalBytesWritten = 0;

    byte *buf = aalloc(mount -> bytesPerCluster, 128);
    if(buf == NULL) return 0;

    while(bytesLeft > 0) {
        if(!readCluster(mount, buf, currentCluster)) {
            free(buf);
            return 0;
        }

        u64 toWrite = bytesLeft > mount -> bytesPerCluster - byteOffset ?
            mount -> bytesPerCluster - byteOffset :
            bytesLeft;

        memcpy(buf + byteOffset, buffer + totalBytesWritten, toWrite);
        if(!writeCluster(mount, buf, currentCluster)) {
            free(buf);
            return totalBytesWritten;
        }

        bytesLeft -= toWrite;
        totalBytesWritten += toWrite;
        byteOffset = 0;

        currentCluster = getNextCluster(mount, currentCluster);
    }

    free(buf);
    return totalBytesWritten;
}

void closeFAT32File(FILE *file) {
    destroyFAT32Entry((FAT32Entry *) file -> internal);
    free(file);
}


boolean FAT32EntryExists(FSMount *mount, const String path) {
    FAT32Mount *fatMount = (FAT32Mount *) mount -> internal;
    FAT32Entry *entry = findFAT32Entry(fatMount, path, fatMount -> rootCluster);
    if(entry == NULL) return false;

    destroyFAT32Entry(entry);
    return true;
}


boolean deleteFAT32Entry(FSMount *mount, const String path) {
    FAT32Mount *fatMount = (FAT32Mount *) mount -> internal;
    FAT32Entry *entry = findFAT32Entry(fatMount, path, fatMount -> rootCluster);
    if(entry == NULL) return false;

    entry -> name[0] = 0xE5;
    if(!writeFAT32Entry(fatMount, entry, true)) {
        destroyFAT32Entry(entry);
        return false;
    }

    if(!resetFAT32ClusterChain(fatMount, entry -> cluster)) {
        destroyFAT32Entry(entry);
        return false;
    }

    destroyFAT32Entry(entry);
    return true;
}

boolean moveFAT32Entry(FSMount *mount, const String oldPath, const String newPath) {
    FAT32Mount *fatMount = (FAT32Mount *) mount -> internal;
    FAT32Entry *entry = findFAT32Entry(fatMount, oldPath, fatMount -> rootCluster);
    if(entry == NULL) return false;

    printf("`%s`, `%s`\n", oldPath, newPath);

    FAT32Entry *desti = findFAT32Entry(fatMount, newPath, fatMount -> rootCluster);
    if(desti != NULL) {
        printf("desti already exists\n");

        destroyFAT32Entry(desti);
        destroyFAT32Entry(entry);
        return false;
    }

    FAT32EntryInfo info = {
        .cluster = entry -> cluster,
        .size = entry -> size,
        .attr = entry -> attr
    };

    FAT32Entry *newEntry = createFAT32Entry(fatMount, newPath, info);
    if(newEntry == NULL) {
        destroyFAT32Entry(entry);
        return false;
    }

    entry -> name[0] = 0xE5;
    if(!writeFAT32Entry(fatMount, entry, true)) {
        destroyFAT32Entry(newEntry);
        destroyFAT32Entry(entry);
        return false;
    }

    destroyFAT32Entry(newEntry);
    destroyFAT32Entry(entry);
    return true;
}


boolean makeFAT32Dir(FSMount *mount, const String relPath) {
    FAT32Mount *fatMount = (FAT32Mount *) mount -> internal;

    FAT32Entry *existing = findFAT32Entry(fatMount, relPath, fatMount -> rootCluster);
    if(existing != NULL) {
        destroyFAT32Entry(existing);
        return false;
    }

    FAT32Entry *entry = createFAT32Entry(fatMount, relPath, (FAT32EntryInfo) {0, ENDOFCHAIN, ATTRIBUTE_DIRECTORY});
    if(entry == NULL) return false;

    destroyFAT32Entry(entry);
    return true;
}

Dir *openFAT32Dir(FSMount *mount, const String relPath) {
    FAT32Mount *fatMount = (FAT32Mount *) mount -> internal;

    FAT32Iterator *iter = createFAT32PathIterator(fatMount, relPath);
    if(iter == NULL) return NULL;

    Dir *dir = malloc(sizeof(Dir));
    if(dir == NULL) {
        destroyFAT32Iterator(iter);
        return NULL;
    }

    dir -> last = NULL;
    dir -> internal = iter; 
    dir -> mount = mount;

    return dir;
}

DirEntry *readFAT32Dir(Dir *dir) {
    FAT32Iterator *iter = (FAT32Iterator *) dir -> internal;
    FAT32Mount *mount = (FAT32Mount *) dir -> mount -> internal;

    FAT32Entry *entry = nextFAT32Entry(mount, iter);
    if(entry == NULL) return NULL;

    DirEntry *dentry = malloc(sizeof(DirEntry));
    if(dentry == NULL) {
        destroyFAT32Entry(entry);
        return NULL;
    }

    if(dir -> last != NULL && dir -> last -> internal != NULL)
        destroyFAT32Entry(dir -> last -> internal);

    dir -> last = dentry;

    dentry -> internal = entry;
    dentry -> size = entry -> size;
    dentry -> name = entry -> name;
    dentry -> type = (entry -> attr & ATTRIBUTE_DIRECTORY) != 0 ?
        DIR_ENTRY_DIR : DIR_ENTRY_FILE;

    return dentry;
}

boolean closeFAT32Dir(Dir *dir) {
    if(dir == NULL) return false;

    if(dir -> last != NULL && dir -> last -> internal != NULL)
        destroyFAT32Entry(dir -> last -> internal);

    destroyFAT32Iterator(dir -> internal);
    free(dir);

    return true;
}


boolean RegisterFAT32FileSystem() {
    FileSystem *fs = malloc(sizeof(FileSystem));
    if(fs == NULL) return false;

    fs -> probe   = probeFAT32FileSystem;
    fs -> mount   = mountFAT32FileSystem;
    fs -> unmount = unmountFAT32FileSystem;

    fs -> fopen  = openFAT32File;
    fs -> fread  = readFAT32File;
    fs -> fwrite = writeFAT32File;
    fs -> fclose = closeFAT32File;

    fs -> exists = FAT32EntryExists;

    fs -> remove = deleteFAT32Entry;
    fs -> rename = moveFAT32Entry;

    fs -> dmake  = makeFAT32Dir;
    fs -> dopen  = openFAT32Dir;
    fs -> dread  = readFAT32Dir;
    fs -> dclose = closeFAT32Dir;

    return RegisterFileSystem(fs);
}