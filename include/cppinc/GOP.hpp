// 版权所有©XINGJI Studios 2017-2024 保留所有权利。  
// XJ380图像头文件
//GOP.hpp
#ifndef _GOP_HPP_  
#define _GOP_HPP_  
  
#include "../global/stdint.h"
#include "../cinc/fbc.h"
struct PixelColor {  
    uint8_t r;
    uint8_t g;
    uint8_t b;
};  
  
class PixelWriter {  
public:  
    PixelWriter(const FrameBufferConfig& config) : config(config) {}  
    virtual ~PixelWriter() = default;  
    virtual void Write(int x, int y, const PixelColor& c) = 0;  
  
protected:  
    uint8_t* PixelAt(int x, int y) {  
        return config.frame_buffer + 4 * (config.pixels_per_scan_line * y + x);  
    }  
  
private:  
    const FrameBufferConfig& config;  
};  
  
class RGBRWriter : public PixelWriter {  
public:  
    using PixelWriter::PixelWriter; // 继承构造函数  
  
    void Write(int x, int y, const PixelColor& c) override {  
        auto p = PixelAt(x, y);  
        p[0] = c.r;  
        p[1] = c.g;  
        p[2] = c.b; // 注意：这里实际上是RGB，但类名可能是为了某种特定格式  
    }  
};  
  
class BGRRWriter : public PixelWriter {  
public:  
    using PixelWriter::PixelWriter; // 继承构造函数  
  
    void Write(int x, int y, const PixelColor& c) override {  
        auto p = PixelAt(x, y);  
        p[0] = c.b;  
        p[1] = c.g;  
        p[2] = c.r; // BGRR格式  
    }  
};  

#endif