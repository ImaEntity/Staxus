#include "pci.h"
#include <types.h>

#define PCI_CONFIG_OUTPORT 0xCF8
#define PCI_CONFIG_INPORT  0xCFC

dword readConfigDword(byte bus, byte slot, byte func, byte offset) {
    dword address = (dword) (
        (bus << 16)     |
        (slot << 11)    |
        (func << 8)     |
        (offset & 0xFC) |
        ((dword) 0x80000000)
    );

    asm volatile("outl %0, %1" :: "a"(address), "Nd"(PCI_CONFIG_OUTPORT));

    dword ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(PCI_CONFIG_INPORT));
    return ret;
}

PCIDevice PCIFindDevice(byte classCode, byte subClass, byte progIF) {
    PCIDevice device = {.found = false};

    for(byte bus = 0; bus < 256; bus++) {
        for(byte slot = 0; slot < 32; slot++) {
            for(byte func = 0; func < 8; func++) {
                dword data = readConfigDword(bus, slot, func, 0x08);
                
                byte prog = (data >>  8) & 0xFF;
                byte sub  = (data >> 16) & 0xFF;
                byte cls  = (data >> 24) & 0xFF;
                
                if(cls != classCode || sub != subClass || prog != progIF)
                    continue;
                
                device.found = true;
                device.bus   = bus;
                device.slot  = slot;
                device.func  = func;

                return device;
            }
        }
    }

    return device;
}

dword PCIReadBAR(PCIDevice device, int bar) {
    return readConfigDword(
        device.bus,
        device.slot,
        device.func,
        0x10 + (bar * 4)
    );
}