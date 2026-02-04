#include "ahci.h"
#include <types.h>

#include <bus/pci.h>
#include <memory/alloc.h>
#include <memory/utils.h>
#include <video/print.h>

#define AHCI_SIG_NULL    0x00000000
#define AHCI_SIG_SATA    0x00000101
#define AHCI_SIG_SATAPI  0xEB140101
#define AHCI_SIG_PORTMUL 0x96690101
#define AHCI_SIG_ENCLOSE 0xC33C0101

#define ATA_CMD_IDENTIFY  0xEC

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

#define FIS_TYPE_REG_H2D 0x27

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ  0x08

typedef volatile struct {
    dword dba;
    dword dbau;
    dword rsv;
    dword dbc;
} HBA_PRDT_ENTRY;

typedef volatile struct {
    byte fis_type;
    byte pmport;
    byte command;
    byte featurel;
    byte lba0;
    byte lba1;
    byte lba2;
    byte device;
    byte lba3;
    byte lba4;
    byte lba5;
    byte featureh;
    byte countl;
    byte counth;
    byte icc;
    byte control;
    byte rsv[4];
} FIS_REG_H2D;

typedef volatile struct {
    byte           cfis[64]; // command FIS
    byte           acmd[16]; // ATAPI cmd (if used)
    byte           rsv[48];  // reserved
    HBA_PRDT_ENTRY prdt[0];  // PRDT entries follow
} HBA_CMD_TBL;

typedef volatile struct {
     word flags;
     word prdtl;  // PRDT length
    dword prdbc;  // PRD byte count
    dword ctba;   // command table base addr
    dword ctbau;
    dword rsv[4];
} HBA_CMD_HEADER;

AHCIController FindAHCIController() {
    AHCIController controller = {.baseAddrReg = 0};
    PCIDevice device = PCIFindDevice(
        0x01, // mass storage
        0x06, // SATA
        0x01  // AHCI
    );

    if(!device.found)
        return controller;

    dword abar = PCIReadBAR(device, 5);
    HBA_MEM *hba = (HBA_MEM *) (qword) (abar & ~ 0x0F);
    if(hba == NULL)
        return controller;

    controller.baseAddrReg = (dword *) hba;
    controller.portsImplemented = hba -> pi;

    byte count = 0;
    for(int i = 0; i < 32; i++)
        if(hba -> pi & (1 << i)) count++;

    controller.portCount = count;
    return controller;
}

dword getPortSignature(HBA_PORT *port) {
    u8 ipm = (port -> ssts >> 8) & 0x0F;
    u8 det = port -> ssts & 0x0F;

    if(det != HBA_PORT_DET_PRESENT) return AHCI_SIG_NULL;
    if(ipm != HBA_PORT_IPM_ACTIVE) return AHCI_SIG_NULL;

    return port -> sig;
}

void stopCommandEngine(HBA_PORT *port) {
    port -> cmd &= ~0x0001;
    port -> cmd &= ~0x0010;

    while(port -> cmd & 0x4000);
    while(port -> cmd & 0x8000);
}

void startCommandEngine(HBA_PORT *port) {
    while(port -> cmd & 0x8000);

    port -> cmd |= 0x0010;
    port -> cmd |= 0x0001;
}

int findCommandSlot(HBA_PORT *port) {
    dword slots = port -> sact | port -> ci;
    for(int i = 0; i < 32; i++)
        if((slots & (1 << i)) == 0) return i;

    return -1;
}

boolean sendPortCommand(
    HBA_PORT *port,
    u8 ataCommand,
    u64 lba, u16 count,
    void *buffer
) {
    int slot = findCommandSlot(port);
    if(slot == -1) return false;

    HBA_CMD_HEADER *hdr = (HBA_CMD_HEADER *) ((qword) port -> clbu << 32 | port -> clb);
    HBA_CMD_TBL *tbl = (HBA_CMD_TBL *) ((qword) hdr[slot].ctbau << 32 | hdr[slot].ctba);

    memset((void *) tbl, 0, sizeof(HBA_CMD_TBL));

    tbl -> prdt[0].dba = (qword) buffer & 0xFFFFFFFF;
    tbl -> prdt[0].dbau = (qword) buffer >> 32;
    tbl -> prdt[0].dbc = (count * 512) - 1;

    hdr[slot].prdtl = 1;
    hdr[slot].prdbc = 0;

    FIS_REG_H2D *fis = (FIS_REG_H2D *) (&tbl -> cfis);
    fis -> fis_type = FIS_TYPE_REG_H2D;
    fis -> pmport = 0x80;
    fis -> command = ataCommand;

    fis -> lba0 = (u8) (lba >>  0);
    fis -> lba1 = (u8) (lba >>  8);
    fis -> lba2 = (u8) (lba >> 16);
    fis -> lba3 = (u8) (lba >> 24);
    fis -> lba4 = (u8) (lba >> 32);
    fis -> lba5 = (u8) (lba >> 40);

    fis -> device = 1 << 6;
    fis -> countl = count & 0xFF;
    fis -> counth = (count >> 8) & 0xFF;

    while((port -> tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) != 0);

    port -> is = -1;
    port -> ci = 1 << slot;

    while(port -> ci & (1 << slot));
    if((port -> is & 0x40000000) != 0) return false;

    return true;
}

void toHumanReadable(wString result, u64 bytes);
void InitializeAHCIController(AHCIController controller) {
    HBA_MEM *hba = (HBA_MEM *) controller.baseAddrReg;

    printf(L"Initalizing AHCI controller\r\n");

    int driveCount = 0;
    for(int i = 0; i < 32; i++) {
        if((hba -> pi & (1 << i)) == 0) continue;
        HBA_PORT *port = hba -> ports + i;

        dword signature = getPortSignature(port);
        if(signature == AHCI_SIG_NULL) continue;

        stopCommandEngine(port);

        void *cmdList = aalloc(1024, 1024);
        memset(cmdList, 0, 1024);
        port -> clb = (qword) cmdList & 0xFFFFFFFF;
        port -> clbu = (qword) cmdList >> 32;

        void *fis = aalloc(256, 256);
        memset(fis, 0, 256);
        port -> fb = (qword) fis & 0xFFFFFFFF;
        port -> fbu = (qword) fis >> 32;

        HBA_CMD_HEADER *cmdHeader = (HBA_CMD_HEADER *) cmdList;
        for(int j = 0; j < 32; j++) {
            cmdHeader[j].prdtl = 1;

            void *cmdTable = aalloc(256, 128);
            memset(cmdTable, 0, 256);

            cmdHeader[j].ctba = (qword) cmdTable & 0xFFFFFFFF;
            cmdHeader[j].ctbau = (qword) cmdTable >> 32;
        }
        
        startCommandEngine(port);

        word *identify_buf = aalloc(512, 128);
        if(!sendPortCommand(port, ATA_CMD_IDENTIFY, 0, 1, identify_buf)) {
            printf(L"Failed to identify drive %d\r\n", i);
            free(identify_buf);
            continue;
        }

        controller.drives[driveCount].port = port;
        controller.drives[driveCount].flags |= AHCI_DRIVE_PRESENT;

        u32 lba28 = (u32) identify_buf[61] << 16 | identify_buf[60];
        u64 lba48 = 
            (u64) identify_buf[103] << 48 |
            (u64) identify_buf[102] << 32 |
            (u64) identify_buf[101] << 16 |
            (u64) identify_buf[100];

        boolean hasLba48 = (identify_buf[83] & 0x400) != 0;
        if(hasLba48) controller.drives[driveCount].flags |= AHCI_DRIVE_LBA48;

        controller.drives[driveCount].sectorCount = hasLba48 ? lba48 : lba28;
        
        dword sectorSize = 512;
        if(identify_buf[106] & 0x1000)
            sectorSize = ((dword) identify_buf[117] << 16 | identify_buf[118]) << 1;

        controller.drives[driveCount].sectorSize = sectorSize;
        driveCount++;

        free(identify_buf);

        wchar tmp[100];
        toHumanReadable(tmp, (hasLba48 ? lba48 : lba28) * sectorSize);
        printf(
            L"Found AHCI drive with %d blocks of size %d (%s)\r\n",
            hasLba48 ? lba48 : lba28,
            sectorSize,
            tmp
        );
    }
}