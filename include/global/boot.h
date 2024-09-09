// 我也不知道该放哪，应该不会有什么冲突罢，那就先放全局了
#ifndef _BOOT_H_
#define _BOOT_H_
#include "../efi.h"
#include "./memory.h"
typedef struct
{
    MEMORY_MAP MemoryMap;
} BOOT_CONFIG;
#endif
