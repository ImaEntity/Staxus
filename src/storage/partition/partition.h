#ifndef HH_PARTITION
#define HH_PARTITION

#include <types.h>
#include <storage/block.h>

#define MAX_PARTITION_CONTROLLERS 16

typedef struct {
    boolean (*probe)(BlockDevice *dev);
    u32 (*register_blocks)(BlockDevice *dev);
} PartitionController;

typedef struct {
    BlockDevice *parent;
    u64 lbaStart;
    byte type;
} Partition;

boolean RegisterPartitionController(PartitionController *prtr);
u32 RegisterPartitionBlocks(BlockDevice *device);

void ApplyGenericPartitionData(BlockDevice *parent, BlockDevice *device);

#endif