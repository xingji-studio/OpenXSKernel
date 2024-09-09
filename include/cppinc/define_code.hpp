#ifndef _DEFINE_CODE_HPP_
#define _DEFINE_CODE_HPP_

#include "../include.h"

#include"../efi.h"

// graphics/draw.cpp
void rect(const FrameBufferConfig &fbc, int x1, int y1, int x2, int y2, const PixelColor& c);
void dot(const FrameBufferConfig &fbc, int x, int y, const PixelColor& c);
void line(const FrameBufferConfig &fbc, int x0, int y0, int x1, int y1, const PixelColor& c);
void circle(const FrameBufferConfig &fbc,int centerX, int centerY, int radius,int solid,const PixelColor& c);
EFI_STATUS ShowBMP(struct BMP_PIXEL* pixels, struct BITMAPINFOHEADER* infoHeader, const FrameBufferConfig& fbc);
EFI_STATUS LoadBMPFile(CHAR16* filename, struct BITMAPINFOHEADER* infoHeader, struct BMP_PIXEL** pixels);
void bmp(const FrameBufferConfig& fbc,struct EFI_SYSTEM_TABLE* ST, CHAR16 filename[], const UINT32 offsetX_, const UINT32 offsetY_);

// graphics/desktop.cpp
void init_desktop(const FrameBufferConfig &fbc);
void init_dock(const FrameBufferConfig &fbc);
void init_shortcut_dock(const FrameBufferConfig &fbc);
void draw_logo(const FrameBufferConfig &fbc, int xi, int yi);
void draw_studio_logo(const FrameBufferConfig &fbc, int xi, int yi);

//graphics/heap_malloc.cpp
void* HEAP_malloc(uint32_t size);
void HEAP_free(void *ptr);

// font/font.cpp
void WriteAscii(const FrameBufferConfig &fbc, int x, int y, char ch, const PixelColor &c);
void WriteString(const FrameBufferConfig &fbc, int x, int y, const char *s, const PixelColor &c);
void WriteHex(const FrameBufferConfig &fbc, int x, int y, unsigned long long hex, const PixelColor &c);

// kernel/wsod.cpp
void WSOD_Cannot_Alloc_MemoryMap(const FrameBufferConfig &fbc);
void WSOD_Cannot_Alloc_MemoryMap_2(const FrameBufferConfig &fbc);

// kernel/memorym.cpp
void init_MemoryManager(const FrameBufferConfig &fbc, BOOT_CONFIG *BootConfig);

#endif