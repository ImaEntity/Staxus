#ifndef HH_DEVICE_BLOCK
#define HH_DEVICE_BLOCK

#include <types.h>

#define MAX_BLOCK_DEVICES 256

#define BLOCK_DEVICE_FLAG_PHYSICAL 0b00000001

typedef struct _BlockDevice BlockDevice;
struct _BlockDevice {
    byte flags;
    wString model;
    void *internal;
    u64 sectorCount;
    u16 sectorSize;

    boolean (*write)(BlockDevice *device, void *buffer, u64 sector, u64 count);
    boolean (*read)(BlockDevice *device, void *buffer, u64 sector, u64 count);
};

boolean ReadBlockDevice(BlockDevice *device, void *buffer, u64 sector, u64 count);
boolean WriteBlockDevice(BlockDevice *device, void *buffer, u64 sector, u64 count);

boolean RegisterBlockDevice(BlockDevice *device);
BlockDevice **GetBlockDevices(u16 *count);

#endif