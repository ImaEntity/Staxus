#ifndef HH_FILESYS
#define HH_FILESYS

#include <types.h>
#include <storage/block.h>

#define MAX_FILE_SYSTEMS 64
#define MAX_MOUNTED_DEVICES 64

#define FILE_DIRECTORY 0x01
#define FILE_READ_ONLY 0x02

#define FILE_OPEN_READ   0x01
#define FILE_OPEN_WRITE  0x02
#define FILE_OPEN_CREATE 0x04
#define FILE_OPEN_TRUNC  0x08
#define FILE_OPEN_APPEND 0x10

#define DIR_ENTRY_FILE 0
#define DIR_ENTRY_DIR 1

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct _FileSystem FileSystem;
typedef struct {
    void *internal;
    FileSystem *fs;
    BlockDevice *device;
    String mountPoint;
} FSMount;

typedef struct {
    FSMount *mount;
    void *internal;
    byte flags;
    u64 size;
    u64 ptr;
} FILE;

// typedef struct {
//     String name;
//     u64 size;
//     u64 accessed;
//     u64 modified;
//     u64 created;
//     byte flags;
// } FileStats;

typedef struct {
    void *internal;
    String name;
    byte type;
    u64 size;
} DirEntry;

typedef struct {
    DirEntry *last;
    FSMount *mount;
    void *internal;
} Dir;

struct _FileSystem {
    boolean (*probe)(BlockDevice *dev);
    void   *(*mount)(BlockDevice *dev, const String mountPoint);
    void    (*unmount)(FSMount *mount);

    FILE   *(*fopen)(FSMount *mount, const String relPath, byte flags);
    u64     (*fread)(FILE *file, void *buffer, u64 offset, u64 length);
    u64     (*fwrite)(FILE *file, void *buffer, u64 offset, u64 length);
    void    (*fclose)(FILE *file);

    boolean (*exists)(FSMount *mount, const String path);

    boolean (*rename)(FSMount *mount, const String oldPath, const String newPath);
    boolean (*remove)(FSMount *mount, const String path);

    boolean   (*dmake)(FSMount *mount, const String relPath);
    Dir      *(*dopen)(FSMount *mount, const String relPath);
    DirEntry *(*dread)(Dir *dir);
    boolean   (*dclose)(Dir *dir);
};

boolean RegisterFileSystem(FileSystem *fs);

boolean MountFileSystem(BlockDevice *device, const String mountPoint);
boolean UnmountFileSystem(const String mountPoint);

FILE *fopen(const String filepath, String mode);
u64 fread(void *buffer, u64 size, u64 count, FILE *file);
u64 fwrite(void *buffer, u64 size, u64 count, FILE *file);
u64 fprintf(FILE *fp, String fmt, ...);
boolean fclose(FILE *file);

boolean exists(const String path);

u64 ftell(FILE *file);
boolean fseek(FILE *file, u64 offset, int origin);
void rewind(FILE *file);

boolean rename(const String old, const String new);
boolean remove(const String path);

boolean mkdir(const String dirpath);
Dir *opendir(const String dirpath);
DirEntry *readdir(Dir *dir);
boolean closedir(Dir *dir);

String dirname(String path);
String basepath(String path);

#endif