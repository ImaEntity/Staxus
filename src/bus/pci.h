#ifndef HH_PCI
#define HH_PCI

#include <types.h>

typedef struct {
    boolean found;
    byte bus;
    byte slot;
    byte func;
} PCIDevice;

PCIDevice PCIFindDevice(byte classCode, byte subClass, byte progIF);
dword     PCIReadBAR(PCIDevice device, int bar);

#endif