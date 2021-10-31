#include "../include/renderer.h"
#include <cstring>

void ClearColor(uint8_t r, uint8_t g, uint8_t b)
{
    Platform platform = GetCurrentPlatform();
    uint8_t *mem = platform.colorBuffer.buffer;
    for (int i = 0; i < platform.colorBuffer.height; ++i)
    {
        for (int j = 0; j < platform.colorBuffer.width; ++j)
        {
            *mem++ = b;
            *mem++ = g;
            *mem++ = r;
            mem++;
        }
    }
}

void FastClearColor(uint8_t r, uint8_t g, uint8_t b)
{
    // Noice this approach is around 15 times faster than the non optimized one
    // Might write this in assembly lol
    // Will be practice on assembly too
    Platform platform = GetCurrentPlatform();
    uint8_t *mem = platform.colorBuffer.buffer;
    for (int i = 0; i < platform.colorBuffer.width; ++i)
    {
        *mem++ = b;
        *mem++ = g;
        *mem++ = r;
        *mem++ = 0;
    }
    mem             = platform.colorBuffer.buffer;

    uint32_t filled = 1;
    uint32_t stride = platform.colorBuffer.width * platform.colorBuffer.noChannels;

    // __debugbreak();
    while (filled < platform.colorBuffer.height)
    {
        uint32_t remaining = platform.colorBuffer.height - filled;
        if (filled < remaining)
        {
            memcpy((void *)(mem + filled * stride), mem, filled * stride);
            filled = filled * 2;
        }
        else
        {
            memcpy((void *)(mem + filled * stride), mem, remaining * stride);
            filled = filled + remaining;
        }
    }
}