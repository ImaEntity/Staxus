#ifndef HH_STORAGE_AHCI
#define HH_STORAGE_AHCI

#include <types.h>
#include <storage/block.h>

typedef volatile struct {
    dword clb;       // 0x00 command list base
    dword clbu;      // 0x04
    dword fb;        // 0x08 FIS base
    dword fbu;       // 0x0C
    dword is;        // 0x10 interrupt status
    dword ie;        // 0x14 interrupt enable
    dword cmd;       // 0x18 command + status
    dword rsv0;
    dword tfd;       // 0x20 task file data
    dword sig;       // 0x24 signature
    dword ssts;      // 0x28 SATA status
    dword sctl;      // 0x2C SATA control
    dword serr;      // 0x30 SATA error
    dword sact;      // 0x34
    dword ci;        // 0x38 command issue
    dword sntf;      // 0x3C
    dword fbs;       // 0x40
    dword rsv1[11];
    dword vendor[4];
} HBA_PORT;

typedef volatile struct {
    dword    cap;      // 0x00: Host capability
    dword    ghc;      // 0x04: Global host control
    dword    is;       // 0x08: Interrupt status
    dword    pi;       // 0x0C: Ports implemented
    dword    vs;       // 0x10: Version
    dword    ccc_ctl;  // 0x14: Command completion coalescing control
    dword    ccc_pts;  // 0x18: Command completion coalescing ports
    dword    em_loc;   // 0x1C: Enclosure management location
    dword    em_ctl;   // 0x20: Enclosure management control
    dword    cap2;     // 0x24: Extended capabilities
    dword    bohc;     // 0x28: BIOS/OS handoff control and status
    byte     rsv   [0xA0  - 0x2C];
    byte     vendor[0x100 - 0xA0];
    HBA_PORT ports [32];
} HBA_MEM;

typedef struct {
    dword *baseAddrReg;
    dword portsImplemented;
    byte portCount;
} AHCIController;

AHCIController FindAHCIController();
void InitializeAHCIController(AHCIController *controller);

#endif