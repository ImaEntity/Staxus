@echo off

set dismountImg=1
set cleanupTemp=1
set "errs=-Wall -Werror -Wpedantic"
set opt="-O3"

:: Handle parameters and set correct flags
:paramHandler
if not "%1"=="" (
  if "%1"=="--nodismount" set dismountImg=0
  if "%1"=="--noclean" set cleanupTemp=0

  shift
  goto :paramHandler
)

:: Setup
if not exist bin mkdir bin >nul
if not exist tmp mkdir tmp >nul
imdisk -D -m Z: >nul 2>&1

:: make helper test disk or smth
qemu-img create -f raw bin/disk.img 260M > nul
sgdisk bin/disk.img -Z > nul
sgdisk bin/disk.img -o > nul
sgdisk bin/disk.img -n 1:2048:+256M -t 1:0C > nul

:: Create and mount a disk image
qemu-img create -f raw bin/Staxus.img 132M > nul
sgdisk bin/Staxus.img -o >nul
sgdisk bin/Staxus.img -n 1:2048:+128M -t 1:EF00 >nul
imdisk -a -f bin/Staxus.img -m Z: -o rw -o hd -b 1048576 -s 134217728 >nul
format Z: /fs:fat32 /q /y >nul

:: Set up file system on disk
mkdir Z:\EFI\BOOT >nul

:: Compile all utility files
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/fileformat_psf_efi.o        src/file-formats/psf/efi.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/fileformat_psf_krnl.o       src/file-formats/psf/krnl.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/fileformat_pe.o             src/file-formats/pe.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/video_print.o               src/video/print.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/video_gop.o                 src/video/gop.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/string_format.o             src/string/format.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/string_utils.o              src/string/utils.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/memory_map.o                src/memory/map.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/memory_alloc.o              src/memory/alloc.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/memory_manager_identity.o   src/memory/managers/identity.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/memory_utils.o              src/memory/utils.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/io_keyboard.o               src/io/keyboard.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/io_serial.o                 src/io/serial.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/io_mouse.o                  src/io/mouse.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/storage_block.o             src/storage/block.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/storage_ahci.o              src/storage/ahci.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/bus_pci.o                   src/bus/pci.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/partition_mbr.o             src/storage/partition/mbr.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/partition_gpt.o             src/storage/partition/gpt.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/partition_partition.o       src/storage/partition/partition.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/filesystem_filesystem.o     src/file-systems/file-system.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/filesystem_fat32.o          src/file-systems/fat32.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/interrupt_interrupt.o       src/interrupt/interrupt.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/interrupt_apic.o            src/interrupt/apic.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/interrupt_gdt.o             src/interrupt/gdt.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/interrupt_idt.o             src/interrupt/idt.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/interrupt_isr.o             src/interrupt/isr.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/interrupt_irq.o             src/interrupt/irq.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -mno-red-zone -ffreestanding -c -Isrc -o tmp/math_math.o                 src/math/math.c

:: Merge utility files
x86_64-w64-mingw32-gcc -m64 %opt% -s -nodefaultlibs -nostartfiles -nostdlib -Wl,-r -o tmp/filesystem.o   tmp/filesystem_filesystem.o tmp/filesystem_fat32.o
x86_64-w64-mingw32-gcc -m64 %opt% -s -nodefaultlibs -nostartfiles -nostdlib -Wl,-r -o tmp/fileformat.o   tmp/fileformat_psf_krnl.o tmp/fileformat_pe.o
x86_64-w64-mingw32-gcc -m64 %opt% -s -nodefaultlibs -nostartfiles -nostdlib -Wl,-r -o tmp/partition.o    tmp/partition_partition.o tmp/partition_mbr.o tmp/partition_gpt.o
x86_64-w64-mingw32-gcc -m64 %opt% -s -nodefaultlibs -nostartfiles -nostdlib -Wl,-r -o tmp/interrupt.o    tmp/interrupt_interrupt.o tmp/interrupt_apic.o tmp/interrupt_gdt.o tmp/interrupt_idt.o tmp/interrupt_isr.o tmp/interrupt_irq.o
x86_64-w64-mingw32-gcc -m64 %opt% -s -nodefaultlibs -nostartfiles -nostdlib -Wl,-r -o tmp/storage.o      tmp/storage_ahci.o tmp/storage_block.o
x86_64-w64-mingw32-gcc -m64 %opt% -s -nodefaultlibs -nostartfiles -nostdlib -Wl,-r -o tmp/string.o       tmp/string_format.o tmp/string_utils.o
x86_64-w64-mingw32-gcc -m64 %opt% -s -nodefaultlibs -nostartfiles -nostdlib -Wl,-r -o tmp/memory.o       tmp/memory_map.o tmp/memory_alloc.o tmp/memory_utils.o tmp/memory_manager_identity.o
x86_64-w64-mingw32-gcc -m64 %opt% -s -nodefaultlibs -nostartfiles -nostdlib -Wl,-r -o tmp/video.o        tmp/video_gop.o tmp/video_print.o
x86_64-w64-mingw32-gcc -m64 %opt% -s -nodefaultlibs -nostartfiles -nostdlib -Wl,-r -o tmp/math.o         tmp/math_math.o
x86_64-w64-mingw32-gcc -m64 %opt% -s -nodefaultlibs -nostartfiles -nostdlib -Wl,-r -o tmp/bus.o          tmp/bus_pci.o
x86_64-w64-mingw32-gcc -m64 %opt% -s -nodefaultlibs -nostartfiles -nostdlib -Wl,-r -o tmp/io.o           tmp/io_keyboard.o tmp/io_serial.o tmp/io_mouse.o

:: Compile pe test files
gcc -s %opt% %errs% -fno-builtin -nostartfiles -nostdlib -shared -Wl,--entry=0 -o resources/test_dll.dll tmp/video.o tmp/string.o tmp/memory.o tmp/math.o
gcc -s %opt% %errs% -fno-builtin -nostartfiles -nostdlib -L./resources -ltest_dll -o resources/test_pe.exe resources/main.c

:: Compile and link the bootloader, then shove the efi file into the disk image
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -ffreestanding -c -Isrc -o tmp/bootloader.o src/bootloader.c
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -ffreestanding -c -Isrc -Isrc/gnu-efi/inc -Isrc/gnu-efi/inc/x86_64 -Isrc/gnu-efi/inc/protocol -o tmp/efi_data.o src/gnu-efi/efi_data.c
x86_64-w64-mingw32-gcc -m64 %opt% -s -nodefaultlibs -nostartfiles -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o Z:/EFI/BOOT/BOOTX64.EFI tmp/math.o tmp/string.o tmp/fileformat_psf_efi.o tmp/bootloader.o tmp/efi_data.o

:: Compile and link the kernel, then shove it into the disk image
x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -ffreestanding -masm=intel -S -Isrc -o bin/krnltmp.S src/kernel_entry.c

x86_64-w64-mingw32-gcc -m64 %opt% -s %errs% -ffreestanding -c -Isrc -o tmp/krnltmp.o src/kernel_entry.c
x86_64-w64-mingw32-gcc -m64 %opt% -s -nostdlib -nodefaultlibs -nostartfiles -Wl,-e,kernel_entry -Wl,--image-base,0x100000 -o tmp/stxkrnl.exe tmp/krnltmp.o tmp/interrupt.o tmp/math.o tmp/io.o tmp/bus.o tmp/partition.o tmp/fileformat.o tmp/filesystem.o tmp/storage.o tmp/memory.o tmp/string.o tmp/video.o
objcopy -O binary tmp/stxkrnl.exe Z:/stxkrnl.bin

@REM x86_64-w64-mingw32-gcc -m64 -ffreestanding -c -o tmp/kernel.o src/kernel.c
@REM bexlink tmp/kernel.o -e kernel_entry -o Z:/stxkrnl.bex

:: Make sure all resources are present
xcopy resources Z:\resources /E /I >nul

:: Cleanup
if %cleanupTemp%==1 rmdir /s /q tmp >nul
if %dismountImg%==1 imdisk -D -m Z: >nul