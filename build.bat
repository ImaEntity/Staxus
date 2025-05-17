@echo off

set dismountImg=1
set cleanupTemp=1

:: Handle parameters and set correct flags
:paramHandler
if not "%1"=="" (
  if "%1"=="--nodismount" set dismountImg=0
  if "%1"=="--noclean" set cleanupTemp=0

  shift
  goto :paramHandler
)

:: Setup
if not exist bin mkdir bin
if not exist tmp mkdir tmp
imdisk -D -m Z:

:: Create and mount a disk image
fsutil file createnew bin/Staxus.iso 67108864
imdisk -a -f bin/Staxus.iso -m Z: -o rw -o hd
format Z: /fs:fat32 /q /y

:: Set up file system on disk
mkdir Z:\EFI\BOOT

:: Compile and link the bootloader, then shove the efi file into the disk image
x86_64-w64-mingw32-gcc -m64 -Os -s -ffreestanding -c -o tmp/bootloader.o src/bootloader.c
x86_64-w64-mingw32-gcc -m64 -Os -s -ffreestanding -c -Isrc/inc -Isrc/inc/x86_64 -Isrc/inc/protocol -o tmp/efi_data.o src/lib/efi_data.c
x86_64-w64-mingw32-gcc -m64 -Os -s -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o Z:/EFI/BOOT/BOOTX64.EFI tmp/bootloader.o tmp/efi_data.o

:: Compile and link the kernel, then shove it into the disk image
@REM x86_64-w64-mingw32-gcc -m64 -ffreestanding -c -o tmp/kernel.o src/kernel.c
@REM bexlink tmp/kernel.o -e kernel_entry -o Z:/stxkrnl.bex

:: Cleanup
if %cleanupTemp%==1 rmdir /s /q tmp
if %dismountImg%==1 imdisk -D -m Z: