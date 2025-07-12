extern "C" {
#include <efi/efi.h>
}
#include <efi/boot.h>
#include <efi/fbc.h>

extern "C" void KernelMain(const FrameBufferConfig &fbc, EFI_SYSTEM_TABLE &SystemTable, BOOT_CONFIG *BootConfig) {
  // kernel
  while(1) { __asm__("hlt"); };
}
