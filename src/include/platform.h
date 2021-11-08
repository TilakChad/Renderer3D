#pragma once
#include <cassert>
#include <cstdint>

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

    struct
    {
        // No stencil buffer, only depth buffer for now.. with single precision floating point depth
        float *  buffer = nullptr;
        uint32_t width;
        uint32_t height;
    } zBuffer;

    SwapBufferFn SwapBuffer;
    bool         bFirst = true;
    struct
    {
        bool    bMouseScrolled = false;
        int32_t value          = 0;
    } Mouse;
};

void     RendererMainLoop(Platform *platform);
Platform GetCurrentPlatform(void);