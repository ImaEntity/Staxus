# Notice!
To build using the build.bat file you need:
- `sgdisk`
- `qemu-img`
- `imdisk`
- wtvr tf `x86_64-w64-mingw32-gcc` is
- gnu-efi headers in `src/gnu-efi/inc` and `efi_data.c` in `src/gnu-efi`

To run using the run.bat file you need:
- `qemu-system-x86_64`
- `OVMF.fd` in the bin folder