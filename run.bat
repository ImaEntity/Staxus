@echo off

qemu-system-x86_64 -bios bin/OVMF.fd -m 2G ^
-device ahci,id=ahci ^
-drive id=staxus,file=bin/Staxus.img,format=raw,if=none ^
-device ide-hd,drive=staxus,bus=ahci.0,bootindex=0 ^
-drive id=disk,file=bin/disk.img,format=raw,if=none ^
-device ide-hd,drive=disk,bus=ahci.1 ^
-d int,cpu_reset -no-reboot -no-shutdown -D bin/qemu.log ^
-serial stdio