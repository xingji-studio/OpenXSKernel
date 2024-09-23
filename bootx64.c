#include "./include/cinclude.h"
#include "./include/global/memory.h"
#define NULL 0
struct EFI_SYSTEM_TABLE                *ST;  // 系统表
struct EFI_BOOT_SERVICES               *BS;  // 启动信息
struct EFI_GRAPHICS_OUTPUT_PROTOCOL    *GOP; // GOP
struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SFSP;// 文件
struct EFI_LOADED_IMAGE_PROTOCOL       *LIP; // e?
EFI_HANDLE                              IM;

struct EFI_GUID gop_guid = {
    0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}
};
struct EFI_GUID lip_guid = {
    0x5b1b31a1, 0x9562, 0x11d2, {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};
struct EFI_GUID sfsp_guid = {
    0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};
struct EFI_GUID spp_guid = {
    0x31878c87, 0xb75, 0x11d5, {0x9a, 0x4f, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d}
};

void puts(unsigned short *s) {
  ST->ConOut->OutputString(ST->ConOut, s);
}

void *malloc(int buf_size) {
  void              *res;
  unsigned long long status;

  status = BS->AllocatePool(LIP ? LIP->ImageDataType : EfiLoaderData, buf_size,
                            &res); // 分配内存，第一个参数是判断LIP是否存在，如果存在就直接拿过来用
  if (status!=EFI_SUCCESS) return NULL;

  return res;
}

void free(void *buf) {
  BS->FreePool(buf);
}

EFI_STATUS mallocAt(EFI_PHYSICAL_ADDRESS addr, UINTN size) {
  EFI_STATUS           status;
  EFI_PHYSICAL_ADDRESS allocated_addr=addr;
  status = BS->AllocatePages(AllocateAnyPages, LIP ? LIP->ImageDataType : EfiLoaderData,
                             (size + 0xfff) / 0x1000, &allocated_addr);
  return status;
}

void efi_init(EFI_HANDLE ImageHandle, struct EFI_SYSTEM_TABLE *SystemTable) {
  ST = SystemTable;
  BS = SystemTable->BootServices;
  IM = ImageHandle;

  // 初始化
  BS->SetWatchdogTimer(0, 0, 0, NULL); // 别删，不然UEFI会自动重启
  BS->LocateProtocol(&gop_guid, NULL, (void **)&GOP);

  BS->OpenProtocol(ImageHandle, &lip_guid, (void **)&LIP, ImageHandle, NULL,
                   EFI_OPEN_PROTOCOL_GET_PROTOCOL); // 获取LIP
  BS->OpenProtocol(LIP->DeviceHandle, &sfsp_guid, (void **)&SFSP, ImageHandle, NULL,
                   EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL); // 获取SFSP
}
EFI_STATUS GetMMP(MEMORY_MAP *MemoryMap) {
  EFI_STATUS GetMemoryMapStatus = EFI_SUCCESS;

  // 获取内存表
  MemoryMap->Buffer = malloc(MemoryMap->MapSize);

  // 获取内存表
  while( BS->GetMemoryMap(
    &MemoryMap->MapSize,
    (EFI_MEMORY_DESCRIPTOR*)MemoryMap->Buffer,
    &MemoryMap->MapKey,
    &MemoryMap->DescriptorSize,
    &MemoryMap->DescriptorVersion
  ) == EFI_BUFFER_TOO_SMALL) {
    if (MemoryMap->Buffer) {
      free(MemoryMap->Buffer);
      MemoryMap->Buffer = NULL;
    }

    // 重新分配更大的缓冲区
    MemoryMap->Buffer = malloc(MemoryMap->MapSize);
  }

  // 检查是否成功分配了内存
  if (!MemoryMap->Buffer) {
    GetMemoryMapStatus = EFI_OUT_OF_RESOURCES;
  }

  return GetMemoryMapStatus;
}
typedef void (*__attribute__((sysv_abi)) Kernel)(const struct FrameBufferConfig *,
                                                 struct EFI_SYSTEM_TABLE *, BOOT_CONFIG);

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, struct EFI_SYSTEM_TABLE *SystemTable) {
  efi_init(ImageHandle, SystemTable);

  ST->ConOut->ClearScreen(ST->ConOut);

  EFI_PHYSICAL_ADDRESS      entry_addr;
  struct EFI_FILE_PROTOCOL *root, *kernel_file;
  UINTN                     kernel_size   = 4194304;
  void                     *kernel_buffer = malloc(kernel_size);

  SFSP->OpenVolume(SFSP, &root);
  root->Open(root, &kernel_file, L"\\kernel.elf", EFI_FILE_MODE_READ, 0);
  kernel_file->Read(kernel_file, &kernel_size, kernel_buffer); // 读取内核，直接读到kernel_buffer

  Elf64_Ehdr *ehdr = (Elf64_Ehdr *)kernel_buffer;
  UINT64      kernel_first_addr, kernel_last_addr;                   // 计算的首尾
  CalcLoadAddressRange(ehdr, &kernel_first_addr, &kernel_last_addr); // 计算范围

  CopyLoadSegments(ehdr);     // CV
  entry_addr = ehdr->e_entry; // 获取入口点
  //free(kernel_buffer);        // 释放内核文件，丢掉！

  struct FrameBufferConfig config = {
      (UINT8 *)GOP->Mode->FrameBufferBase, GOP->Mode->Info->PixelsPerScanLine,
      GOP->Mode->Info->HorizontalResolution, GOP->Mode->Info->VerticalResolution};

  switch (GOP->Mode->Info->PixelFormat) {
  case PixelRedGreenBlueReserved8BitPerColor: config.pixel_format = kRGBR; break;
  case PixelBlueGreenRedReserved8BitPerColor: config.pixel_format = kBGRR; break;
  default:
    while (1);
  }

  BOOT_CONFIG BootConfig;
  BootConfig.MemoryMap.MapSize = 4096;
  BootConfig.MemoryMap.Buffer = NULL;
  BootConfig.MemoryMap.MapKey = 0;
  BootConfig.MemoryMap.DescriptorSize = 0;
  BootConfig.MemoryMap.DescriptorVersion = 0;

  GetMMP(&BootConfig.MemoryMap);
  
  Kernel kernel = (Kernel)entry_addr;
  kernel(&config, SystemTable, BootConfig); // 滚进去！

  //此处=kernel.elf

  while (1); // 趋势
}
