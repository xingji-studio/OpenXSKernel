// 我也不知道该放哪，应该不会有什么冲突罢，那就先放全局了
#ifndef _MEMORY_H_
#define _MEMORY_H_
#define FREE_MEMORY 1
#define MMIO_MEMORY 2
#define UEFI_MEMORY 3
#pragma pack(16)
#include "../efi.h"
#include "./stdint.h"
typedef struct __packed
{
    UINTN MapSize;
    VOID *Buffer;
    UINTN MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;
} MEMORY_MAP;

typedef struct {
	UINT32 Type;
	UINT64 PhysicalStart;
	UINT64 PageSize;
} OS_MEMORY_DESCRIPTOR;
#endif
