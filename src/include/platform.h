#pragma once
#include <cstdint>
#include <cassert>

using float32 = float;
using float64 = double;

struct Platform;
typedef void (*SwapBufferFn)(void);

struct Platform
{
    // What should a platform have?
    uint32_t height;
    uint32_t width;
    float32  deltaTime;

    struct
    {
        uint8_t *buffer;
        uint32_t width;
        uint32_t height;
        uint8_t  noChannels;
    } colorBuffer;

    SwapBufferFn SwapBuffer;
    bool         bFirst = true;
    struct 
    {
        bool bMouseScrolled = false; 
        int32_t value          = 0;
    } Mouse;
};

void RendererMainLoop(Platform *platform);
Platform GetCurrentPlatform(void);