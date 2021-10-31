#include "./include/platform.h"
#include "./include/rasteriser.h"
#include <cstring>

void RendererMainLoop(Platform *platform)
    {
    // ClearColor(0x80, 0x00, 0x80);
    FastClearColor(0x80, 0x00, 0x80);
    platform->SwapBuffer();
}