#include "./include/platform.h"
#include "./include/rasteriser.h"

#include "./image/PNGLoader.h"
#include <cstring>

#include "./maths/vec.hpp"

static uint32_t catTexture;

void            RendererMainLoop(Platform *platform)
{

    if (platform->bFirst)
    {
        ClearColor(0xB0, 0xB0, 0xB0);
        platform->bFirst              = false;
        static uint32_t target_height = 64;
        float           aspect_ratio  = 1;
        static uint32_t target_width  = 64;
        if (platform->Mouse.bMouseScrolled)
        {
            target_width                   = (target_width + platform->Mouse.value * 8) / aspect_ratio;
            target_height                  = target_height + platform->Mouse.value * 8;
            platform->Mouse.bMouseScrolled = false;
        }
        uint32_t height, width, no_channels, bit_depth;
        uint8_t *png = LoadPNGFromFile("./src/include/fancy.png", &height, &width, &no_channels, &bit_depth);
        if (png)
        {
            std::cout << "\nPNG loading successful with info : \n Width -> " << width << "\nHeight -> " << height
                      << "\nNo. of channels -> " << no_channels << "\nBit depth -> " << bit_depth << std::endl;
            // DrawImage("./src/include/cat.png", platform->colorBuffer.buffer, platform->colorBuffer.width,
            //          platform->colorBuffer.height, platform->colorBuffer.noChannels);
            // free(png);
            catTexture = CreateTexture("./src/include/fancy.png");
            SetActiveTexture(catTexture);
            auto     texture  = GetActiveTexture();
            uint8_t *gaussian = texture.Convolve(Texture::Convolution::EdgeDetection);
            ImageViewer(gaussian, platform->colorBuffer.buffer, width, height, no_channels, platform->colorBuffer.width,
                        platform->colorBuffer.height, platform->colorBuffer.noChannels, target_width, target_height);
            free(png);
            free(gaussian);
        }
        else
            std::cout << "OOOps .. Failed to load the given PNG";
        catTexture = CreateTexture("./src/include/fancy.png");
        if (!catTexture)
            std::cout << "Failed to create cat texture..";
        else
            std::cout << "Successfully created the cat texture..";
    }

    //// ClearColor(0x80, 0x00, 0x80);
    //// FastClearColor(0xC0, 0xC0, 0xC0);
    //// ScreenSpace(-1.0f, -0.5f, 0.5f, 0.5f, 1.0f, 0.0f);
    //// ScreenSpace(0.5f, 0.5f, -1.0f, -0.5f, 1.0f, 0.0f);
    //// ClipSpace(Vec2(-0.5f, -0.5f), Vec2(-0.5f, 0.5f), (0.5f, 0.5f));
    //// ClipSpace(Vec2(- 7.0f, -0.5f),Vec2(0.5f, 0.5f), Vec2(1.0f, 0.0f));
    // float aspect_ratio = platform->width * 1.0f/ platform->height;
    // if (aspect_ratio < 1.5f)
    //    volatile int a = 5;
    // VertexAttrib2D v0 = {Vec2f(-1.0f / aspect_ratio, -1.0f), Vec2f(0.35, 0.35), Vec3u8(0xFF, 0x00, 0x00)};
    // VertexAttrib2D v1 = {Vec2f(-1.0f / aspect_ratio, 1.0f), Vec2f(0.35, 0.65), Vec3u8(0x00, 0xFF, 0x00)};
    // VertexAttrib2D v2 = {Vec2f(1.0f / aspect_ratio, 1.0f), Vec2f(0.65, 0.65), Vec3u8(0x00, 0x00, 0xFF)};

    ////// VertexAttrib2D v0 = {Vec2f(-7.0f, -0, 0), Vec3u8(0xFF, 0x00, 0x00)};
    ////// VertexAttrib2D v1 = {Vec2f(-0.5f, 0, 0), Vec3u8(0x00, 0xFF, 0x00)};
    ////// VertexAttrib2D v2 = {Vec2f(1.0f, 0. 0), Vec3u8(0x00, 0x00, 0xFF)};

    // VertexAttrib2D v3 = {Vec2f(1.0f / aspect_ratio, -1.0f), Vec2f(0.65, 0.35), Vec3u8(0xFF, 0xFF, 0x00)};

    //////  ClipSpace(Vec2(-0.75f, -0.75f), Vec2(-0.75f, 0.75f), Vec2(0.5f, 0.75f));
    ////

    // SetActiveTexture(catTexture);
    // ClipSpace(v0, v1, v2);
    // ClipSpace(v0, v2, v3);
    //  platform->SwapBuffer();

    platform->SwapBuffer();
}