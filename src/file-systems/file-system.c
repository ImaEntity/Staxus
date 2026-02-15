#include "file-system.h"
#include <types.h>

#include <memory/alloc.h>
#include <memory/utils.h>
#include <string/utils.h>
#include <video/print.h>
#include <stdarg.h>

static u32 fileSystemCount = 0;
static FileSystem *fileSystems[MAX_FILE_SYSTEMS];
boolean RegisterFileSystem(FileSystem *fs) {
    if(fileSystemCount >= MAX_FILE_SYSTEMS) return false;
    fileSystems[fileSystemCount++] = fs;
    return true;
}


static u32 mountedCount = 0;
static FSMount *mounted[MAX_MOUNTED_DEVICES];
boolean MountFileSystem(BlockDevice *device, const String mountPoint) {
    for(u32 i = 0; i < mountedCount; i++) {
        if(strcmp(mounted[i] -> mountPoint, mountPoint) == 0)
            return false;
    }

    for(u32 i = 0; i < fileSystemCount; i++) {
        if(!fileSystems[i] -> probe(device)) continue;

        FSMount *mount = malloc(sizeof(FSMount));
        if(mount == NULL) return false;

        mount -> mountPoint = malloc(strlen(mountPoint) + 1);
        if(mount -> mountPoint == NULL) { free(mount); return false; }
        strcpy(mount -> mountPoint, mountPoint);

        void *fsInternal = fileSystems[i] -> mount(device, mountPoint);
        if(fsInternal == NULL) {
            free(mount -> mountPoint);
            free(mount);

            return false;
        }

        mount -> internal = fsInternal;
        mount -> fs = fileSystems[i];
        mount -> device = device;

        mounted[mountedCount++] = mount;
        return true;
    }

    return false;
}

boolean UnmountFileSystem(const String mountPoint) {
    for(u32 i = 0; i < mountedCount; i++) {
        if(strcmp(mounted[i] -> mountPoint, mountPoint) != 0)
            continue;

        mounted[i] -> fs -> unmount(mounted[i]);

        free(mounted[i] -> mountPoint);
        free(mounted[i]);

        mounted[i] = mounted[--mountedCount];
        return true;
    }

    return false;
}


FSMount *getMount(const String filepath, String *relPath) {
    if(filepath == NULL) return NULL;
    if(relPath == NULL) return NULL;

    FSMount *mount = NULL;
    u64 longestMatch = 0;

    for(u32 i = 0; i < mountedCount; i++) {
        u64 mpLen = strlen(mounted[i] -> mountPoint);
        if(mpLen <= longestMatch) continue;

        if(memcmp(filepath, mounted[i] -> mountPoint, mpLen) != 0) continue;
        if(
            filepath[mpLen] != '/' && filepath[mpLen] != 0 &&
            !(mpLen == 1 && filepath[mpLen - 1] == '/')
        ) continue;

        *relPath = filepath + mpLen;
        longestMatch = mpLen;
        mount = mounted[i];
    }

    if(mount == NULL) return NULL;
    return mount;
}


FILE *fopen(const String filepath, String mode) {
    String relPath;
    FSMount *mount = getMount(filepath, &relPath);

    if(mount == NULL) return NULL;

    byte flags = 0;
    if(strchr(mode, 'r') != NULL) flags |= FILE_OPEN_READ;
    if(strchr(mode, 'w') != NULL) flags |= FILE_OPEN_WRITE | FILE_OPEN_CREATE | FILE_OPEN_TRUNC;
    if(strchr(mode, 'a') != NULL) flags |= FILE_OPEN_WRITE | FILE_OPEN_CREATE | FILE_OPEN_APPEND;
    if(strchr(mode, '+') != NULL) flags |= FILE_OPEN_READ  | FILE_OPEN_WRITE;

    return mount -> fs -> fopen(mount, relPath, flags);
}

u64 fread(void *buffer, u64 size, u64 count, FILE *file) {
    if(file == NULL) return false;
    if((file -> flags & FILE_OPEN_READ) == 0) return false;

    if(file -> mount == NULL) return false;
    if(file -> mount -> fs == NULL) return false;
    if(file -> mount -> fs -> fread == NULL) return false;

    u64 read = file -> mount -> fs -> fread(file, buffer, file -> ptr, size * count);

    file -> ptr += read;
    return read / size;
}

u64 fwrite(void *buffer, u64 size, u64 count, FILE *file) {
    if(file == NULL) return false;
    if((file -> flags & FILE_OPEN_WRITE) == 0) return false;

    if(file -> mount == NULL) return false;
    if(file -> mount -> fs == NULL) return false;
    if(file -> mount -> fs -> fwrite == NULL) return false;

    u64 written = file -> mount -> fs -> fwrite(file, buffer, file -> ptr, size * count);

    file -> ptr += written;
    return written / size;
}

u64 fprintf(FILE *fp, String fmt, ...) {
    va_list args;
    va_start(args, fmt);

    static char buf[1024];

    vsprintf(buf, fmt, args);
    u64 len = strlen(buf);
    u64 written = fwrite(buf, 1, len, fp);

    free(buf);
    va_end(args);

    return written;
}

boolean fclose(FILE *file) {
    if(file == NULL) return false;
    if(file -> mount == NULL) return false;
    if(file -> mount -> fs == NULL) return false;
    if(file -> mount -> fs -> fclose == NULL) return false;

    file -> mount -> fs -> fclose(file);
    return true;
}


boolean exists(const String path) {
    String relPath;
    FSMount *mount = getMount(path, &relPath);

    if(mount == NULL) return false;
    if(mount -> fs -> exists == NULL) return false;

    return mount -> fs -> exists(mount, relPath);
}


inline u64 ftell(FILE *file) {
    return file -> ptr;
}

boolean fseek(FILE *file, u64 offset, int origin) {
    if(origin == SEEK_CUR) file -> ptr += offset;
    else if(origin == SEEK_SET) file -> ptr = offset;
    else if(origin == SEEK_END) file -> ptr = file -> size + offset;
    else return false;

    return true;
}

inline void rewind(FILE *file) {
    fseek(file, 0, SEEK_SET);
}


boolean rename(const String old, const String new) {
    String relPathOld;
    FSMount *oldMount = getMount(old, &relPathOld);

    String relPathNew;
    FSMount *newMount = getMount(new, &relPathNew);

    if(oldMount != newMount) {
        if(!oldMount -> fs -> exists(oldMount, relPathOld)) return false;
        if( newMount -> fs -> exists(newMount, relPathNew)) return false;

        FILE *old = oldMount -> fs -> fopen(oldMount, relPathOld, FILE_OPEN_READ);
        if(old == NULL) return false;

        FILE *new = newMount -> fs -> fopen(newMount, relPathNew, FILE_OPEN_CREATE | FILE_OPEN_WRITE);
        if(new == NULL) {
            oldMount -> fs -> fclose(old);
            return false;
        }

        byte *buf = malloc(old -> size);
        if(buf == NULL) {
            oldMount -> fs -> fclose(old);
            newMount -> fs -> fclose(new);
            return false;
        }

        oldMount -> fs -> fread(old, buf, 0, old -> size);
        newMount -> fs -> fwrite(new, buf, 0, old -> size);

        oldMount -> fs -> fclose(old);
        newMount -> fs -> fclose(new);

        return true;
    }

    if(oldMount == NULL) return false;
    if(oldMount -> fs -> rename == NULL) return false;

    return oldMount -> fs -> rename(oldMount, relPathOld, relPathNew);
}

boolean remove(const String path) {
    String relPath;
    FSMount *mount = getMount(path, &relPath);

    if(mount == NULL) return false;
    if(mount -> fs -> remove == NULL) return false;

    return mount -> fs -> remove(mount, relPath);
}


boolean mkdir(const String path) {
    String relPath;
    FSMount *mount = getMount(path, &relPath);

    if(mount == NULL) return false;
    if(mount -> fs -> dmake == NULL) return false;

    return mount -> fs -> dmake(mount, relPath);
}

Dir *opendir(const String dirpath) {
    String relPath;
    FSMount *mount = getMount(dirpath, &relPath);
    
    if(mount == NULL) return NULL;
    return mount -> fs -> dopen(mount, relPath);
}

DirEntry *readdir(Dir *dir) {
    if(dir == NULL) return NULL;
    if(dir -> mount == NULL) return NULL;
    if(dir -> mount -> fs == NULL) return NULL;
    if(dir -> mount -> fs -> dread == NULL) return NULL;

    return dir -> mount -> fs -> dread(dir);
}

boolean closedir(Dir *dir) {
    if(dir == NULL) return false;
    if(dir -> mount == NULL) return false;
    if(dir -> mount -> fs == NULL) return false;
    if(dir -> mount -> fs -> dclose == NULL) return false;

    return dir -> mount -> fs -> dclose(dir);
}


String dirname(String path) {
    String p = path;
    String lastSlash = p;
    while(*p != 0) {
        if(*p == '/') lastSlash = p;
        p++;
    }

    if(lastSlash == path) return p;

    *lastSlash = 0;
    return path;
}

String basepath(String path) {
    String lastSlash = path;
    while(*path != 0) {
        if(*path == '/') lastSlash = path + 1;
        path++;
    }

    return lastSlash;
}
