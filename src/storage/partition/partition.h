#ifndef HH_PARTITION
#define HH_PARTITION

#include <types.h>
#include <storage/block.h>

#define MAX_PARTITION_CONTROLLERS 16
#define MAX_PARTITIONS 1024

typedef struct {
    boolean (*probe)(BlockDevice *dev);
    u32 (*register_blocks)(BlockDevice *dev);
} PartitionController;

typedef struct {
    BlockDevice *parent;
    BlockDevice *device;
    u64 lbaStart;
    u64 lbaEnd;
    byte type;
} Partition;

boolean RegisterPartitionController(PartitionController *prtr);
u32 RegisterPartitionBlocks(BlockDevice *device);

boolean RegisterPartition(Partition *part);
Partition **GetPartitions(u16 *count);

void ApplyGenericPartitionData(BlockDevice *parent, BlockDevice *device);

#endif