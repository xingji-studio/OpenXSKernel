// 版权所有©XINGJI Studios 2017-2024 保留所有权利。
// XJ380 类型定义头文件
// 卧槽不会你连这都看不懂吧？！
#ifndef _STDINT_H_
#define _STDINT_H_

// 规范类型
// 无符号整型
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
// 有符号整型
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
// 浮点型
typedef float float32_t;
typedef double float64_t;

// 自定义类型
// 其他的在efi.h里
// 浮点型
typedef float32_t f32;
typedef float64_t f64;

#define NULL    0

typedef unsigned long int size_t; // compatible to ELF toolchain

#endif