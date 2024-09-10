@echo off
chcp 65001
cls
del /Q /S out\*
del /Q /S esp\*
mkdir out
mkdir esp\EFI\BOOT
..\tools\mingw64\bin\x86_64-w64-mingw32-gcc.exe -Wall -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-unused-variable -Wextra -e efi_main -nostdinc -nostdlib -fno-builtin -Wl,--subsystem,10 -o out\BOOTX64.efi bootx64.c
..\tools\x86_64-elf-tools-windows\bin\x86_64-elf-g++.exe kernel\main.cpp -O2 -Wall -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-unused-variable -g -ffreestanding -fno-exceptions -fno-rtti -std=c++17 -c -o out\main.o
..\tools\x86_64-elf-tools-windows\bin\x86_64-elf-ld.exe -e KernelMain -z norelro --static -o .\out\kernel.elf .\out\main.o
copy out\BOOTX64.efi esp\EFI\BOOT
copy out\kernel.elf esp
..\tools\qemu\qemu-system-x86_64.exe -bios ..\tools\OVMF.fd -m 1024 -net none -drive file=fat:rw:esp,index=0,format=vvfat -serial file:serial.log -device usb-ehci,id=usb -device usb-tablet,bus=usb.0 -device usb-kbd,bus=usb.0