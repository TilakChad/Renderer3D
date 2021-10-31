#include "../../include/rasteriser.h"

#include <algorithm>
#include <compare>

struct Vec2
{
    union {

        struct
        {
            int32_t x;
            int32_t y;
        };
        struct
        {
            int32_t elem[2];
        };
    };
    Vec2() = default;
    Vec2(int32_t x, int32_t y) : x{x}, y{y}
    {
    }
    constexpr auto operator<=>(const Vec2 &vec) const = default;
    Vec2           operator-(const Vec2 &vec)
    {
        return Vec2(x - vec.x, y - vec.y);
    }
    Vec2 operator+(const Vec2 &vec)
    {
        return Vec2(x + vec.x, y + vec.y);
    }
    static int32_t Determinant(const Vec2 &a, const Vec2 &b)
    {
        return a.x * b.y - b.x * a.y;
    }
};

void Rasteriser(int x1, int y1, int x2, int y2, int x3, int y3)
{
    // Its the triangle rasteriser
    // First comes the clipping phase.. anything outside |1| are clipped against the rectangle
    // Clipping will be done in previous phase

    Platform platform = GetCurrentPlatform();

    int      minX     = std::min({x1, x2, x3});
    int      maxX     = std::max({x1, x2, x3});

    int      minY     = std::min({y1, y2, y3});
    int      maxY     = std::max({y1, y2, y3});

    // Assume vectors are in clockwise ordering
    Vec2     v0 = Vec2(x2, y2) - Vec2(x1, y1);
    Vec2     v1 = Vec2(x3, y3) - Vec2(x2, y2);
    Vec2     v2 = Vec2(x1, y1) - Vec2(x3, y3);

    Vec2     vec1 = Vec2(x1, y1);
    Vec2     vec2 = Vec2(x2, y2);
    Vec2     vec3 = Vec2(x3, y3);

    uint8_t *mem;
    
    // potential for paralellization 

    for (int h = minY; h <= maxY; ++h)
    {
        int32_t offset =
            (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width * platform.colorBuffer.noChannels;
        mem = platform.colorBuffer.buffer + offset; 
        mem = mem + minX * platform.colorBuffer.noChannels;
        // This is a very tight loop and need to be optimized heavily even for a suitable framerate
        for (int w = minX; w <= maxX; ++w)
        {
            Vec2 point = Vec2(w, h);
            if (Vec2::Determinant(point - vec1, v0) > 0 && Vec2::Determinant(point - vec2, v1) > 0 &&
                Vec2::Determinant(point - vec3, v2) > 0)
            {
                mem[0] = 0x00;
                mem[1] = 0xFF;
                mem[2] = 0xFF;
                mem[3] = 0x00;
            }
            mem = mem + 4;
        }
    }
}

void ScreenSpace(float x1, float y1, float x2, float y2, float x3, float y3)
{
    Platform platform = GetCurrentPlatform();
    int      scrX1    = static_cast<int>((platform.width - 1) / 2 * (x1 + 1));
    int      scrY1    = static_cast<int>((platform.height - 1) / 2 * (1 + y1));

    int      scrX2    = static_cast<int>((platform.width - 1) / 2 * (x2 + 1));
    int      scrY2    = static_cast<int>((platform.height - 1) / 2 * (1 + y2));

    int      scrX3    = static_cast<int>((platform.width - 1) / 2 * (x3 + 1));
    int      scrY3    = static_cast<int>((platform.height - 1) / 2 * (1 + y3));

    Rasteriser(scrX1, scrY1, scrX2, scrY2, scrX3, scrY3);

    // int      minX     = std::min({scrX1, scrX2, scrX3});
    // int      maxX     = std::max({scrX1, scrX2, scrX3});

    // int      minY     = std::min({scrY1, scrY2, scrY3});
    // int      maxY     = std::max({scrY1, scrY2, scrY3});

    //// (minX,minY) and (maxX, maxY) forms the axis aligned bounding box of the triangle
    // uint8_t *mem;
    // for (int h = minY; h <= maxY; ++h)
    //{
    //    int stride = h * platform.width * platform.colorBuffer.noChannels;
    //    mem        = platform.colorBuffer.buffer + stride;
    //    mem        = mem + minX * platform.colorBuffer.noChannels;
    //    for (int w = minX; w <= maxX; ++w)
    //    {
    //        *mem++ = 0xFF;
    //        *mem++ = 0x00;
    //        *mem++ = 0x00;
    //        *mem++ = 0x00;
    //    }
    //}
}
