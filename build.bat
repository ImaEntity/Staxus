@echo off

set dismountImg=1
set cleanupTemp=1

:: Handle parameters and set correct flags
:paramHandler
if not "%1"=="" (
  if "%1"=="--no-dismount" set dismountImg=0
  if "%1"=="--no-clean" set cleanupTemp=0

  shift
  goto :paramHandler
)

:: Setup
if not exist bin mkdir bin >nul
if not exist tmp mkdir tmp >nul
imdisk -D -m Z: >nul 2>&1

:: Create and mount a disk image
fsutil file createnew bin/Staxus.iso 67108864 >nul
imdisk -a -f bin/Staxus.iso -m Z: -o rw -o hd >nul
format Z: /fs:fat32 /q /y >nul

:: Set up file system on disk
mkdir Z:\EFI\BOOT >nul

:: Compile all utility files
x86_64-w64-mingw32-gcc -m64 -Os -s -Wall -Werror -ffreestanding -c -o tmp/psf.o src/file-formats/psf.c
x86_64-w64-mingw32-gcc -m64 -Os -s -Wall -Werror -ffreestanding -c -o tmp/gop.o src/graphics/gop.c
x86_64-w64-mingw32-gcc -m64 -Os -s -Wall -Werror -ffreestanding -c -o tmp/format.o src/io/format.c
x86_64-w64-mingw32-gcc -m64 -Os -s -Wall -Werror -ffreestanding -c -o tmp/print.o src/io/print.c

:: Compile and link the bootloader, then shove the efi file into the disk image
x86_64-w64-mingw32-gcc -m64 -Os -s -Wall -Werror -ffreestanding -c -o tmp/bootloader.o src/bootloader.c
x86_64-w64-mingw32-gcc -m64 -Os -s -Wall -Werror -ffreestanding -c -Isrc/gnu-efi/inc -Isrc/gnu-efi/inc/x86_64 -Isrc/gnu-efi/inc/protocol -o tmp/efi_data.o src/gnu-efi/efi_data.c
x86_64-w64-mingw32-gcc -m64 -Os -s -nodefaultlibs -nostartfiles -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o Z:/EFI/BOOT/BOOTX64.EFI tmp/psf.o tmp/bootloader.o tmp/efi_data.o

:: Compile and link the kernel, then shove it into the disk image
x86_64-w64-mingw32-gcc -m64 -Os -s -Wall -Werror -ffreestanding -c -o tmp/krnltmp.o src/kernel.c
x86_64-w64-mingw32-gcc -m64 -Os -s -nostdlib -nodefaultlibs -nostartfiles -o tmp/stxkrnl.o tmp/krnltmp.o tmp/format.o tmp/print.o tmp/gop.o -Wl,-N -Wl,-e,kernel_entry
objcopy -O binary tmp/stxkrnl.o Z:/stxkrnl.bin

@REM x86_64-w64-mingw32-gcc -m64 -ffreestanding -c -o tmp/kernel.o src/kernel.c
@REM bexlink tmp/kernel.o -e kernel_entry -o Z:/stxkrnl.bex

:: Make sure all resources are present
xcopy resources Z:\resources /E /I >nul

:: Cleanup
if %cleanupTemp%==1 rmdir /s /q tmp >nul
if %dismountImg%==1 imdisk -D -m Z: >nul