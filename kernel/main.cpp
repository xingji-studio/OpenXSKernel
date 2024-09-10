extern "C" {
#include "../include/efi.h"
}
#include "../include/global/boot.h"
#include "../include/cinc/fbc.h"

extern "C" void KernelMain(const FrameBufferConfig &fbc, EFI_SYSTEM_TABLE &SystemTable, BOOT_CONFIG *BootConfig) {
  // kernel
  while(1) { __asm__("hlt"); };
}
