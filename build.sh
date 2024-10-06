#!/usr/bin/sh
rm -r out esp
mkdir out
mkdir -p esp/EFI/BOOT
x86_64-w64-mingw32-gcc -W -Wno-missing-field-initializers -Wextra -e efi_main -nostdinc -nostdlib -fno-builtin -Wl,--subsystem,10 -fshort-wchar -o out/BOOTX64.efi bootx64.c
g++ kernel/main.cpp -O2 -Wall -g -ffreestanding -fno-exceptions -fno-rtti -std=c++17 -fshort-wchar -c -o out/main.o
ld -e KernelMain -z norelro --static -o ./out/kernel.elf ./out/main.o
cp out/BOOTX64.efi esp/EFI/BOOT
cp out/kernel.elf esp
echo "\033[32m[Done]\033[0m" Compilation complete.
echo
qemu-system-x86_64 -bios OVMF.fd -m 1024 -net none -drive file=fat:rw:esp,index=0,format=vvfat
