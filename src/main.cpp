#include "./include/platform.h"
#include "./include/rasteriser.h"
#include <cstring>

#include "./maths/vec.hpp"

void RendererMainLoop(Platform *platform)
{
    // ClearColor(0x80, 0x00, 0x80);
    FastClearColor(0xC0, 0xC0, 0xC0);
    // ScreenSpace(-1.0f, -0.5f, 0.5f, 0.5f, 1.0f, 0.0f);
    // ScreenSpace(0.5f, 0.5f, -1.0f, -0.5f, 1.0f, 0.0f);
    // ClipSpace(Vec2(-0.5f, -0.5f), Vec2(-0.5f, 0.5f), (0.5f, 0.5f));
    ClipSpace(Vec2(- 7.0f, -0.5f),Vec2(0.5f, 0.5f), Vec2(1.0f, 0.0f));
    platform->SwapBuffer();
}