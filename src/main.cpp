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
    // ClipSpace(Vec2(- 7.0f, -0.5f),Vec2(0.5f, 0.5f), Vec2(1.0f, 0.0f));

    //VertexAttrib2D v0 = {Vec2f(-0.75f, -0.75f), Vec2f(0, 0), Vec3u8(0xFF, 0x00, 0x00)};
    //VertexAttrib2D v1 = {Vec2f(-0.75f, 0.75f), Vec2f(0, 0), Vec3u8(0x00, 0xFF, 0x00)};
    //VertexAttrib2D v2 = {Vec2f( 0.75f,  0.75f), Vec2f(0, 0), Vec3u8(0x00, 0x00, 0xFF)};

    VertexAttrib2D v0 = {Vec2f(-7.0f, -0.5f), Vec2f(0, 0), Vec3u8(0xFF, 0x00, 0x00)};
    VertexAttrib2D v1 = {Vec2f(-0.5f, 0.5f), Vec2f(0, 0), Vec3u8(0x00, 0xFF, 0x00)};
    VertexAttrib2D v2 = {Vec2f(1.0f, 0.0f), Vec2f(0, 0), Vec3u8(0x00, 0x00, 0xFF)};

    VertexAttrib2D v3 = {Vec2f(0.75f, -0.75f), Vec2f(0, 0), Vec3u8(0xFF, 0xFF, 0x00)};

   //  ClipSpace(Vec2(-0.75f, -0.75f), Vec2(-0.75f, 0.75f), Vec2(0.5f, 0.75f));
   
    ClipSpace(v3, v1, v2);
    // ClipSpace(v0, v2, v3);
    platform->SwapBuffer();
}