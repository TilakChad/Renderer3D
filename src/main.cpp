#include "./include/platform.h"
#include "./include/rasteriser.h"

#include "./image/PNGLoader.h"
#include "./maths/vec.hpp"

// TODO : Cleanup
// TODO : Make it simpler
//
// TODO : Allow Vertex Transform
//
// TODO : Optimizing Rasterisation algorithm
// TODO : Start 3D pipeline

static uint32_t      catTexture;
static uint32_t      fancyTexture;
static RenderDevice *Device = nullptr;

void                 RendererMainLoop(Platform *platform)
{

    if (platform->bFirst)
    {
        ClearColor(0xB0, 0xB0, 0xB0);
        // Initialize the rendering device

        platform->bFirst = false;
        {

            static uint32_t target_height = 64;
            float           aspect_ratio  = 1;
            // static uint32_t target_width  = 64;
            // if (platform->Mouse.bMouseScrolled)
            //{
            //    target_width                   = (target_width + platform->Mouse.value * 8) / aspect_ratio;
            //    target_height                  = target_height + platform->Mouse.value * 8;
            //    platform->Mouse.bMouseScrolled = false;
            //}
            // uint32_t height, width, no_channels, bit_depth;
            // uint8_t *png = LoadPNGFromFile("./src/include/fancy.png", &height, &width, &no_channels, &bit_depth);
            // if (png)
            //{
            //    std::cout << "\nPNG loading successful with info : \n Width -> " << width << "\nHeight -> " << height
            //              << "\nNo. of channels -> " << no_channels << "\nBit depth -> " << bit_depth << std::endl;
            //    // DrawImage("./src/include/cat.png", platform->colorBuffer.buffer, platform->colorBuffer.width,
            //    //          platform->colorBuffer.height, platform->colorBuffer.noChannels);
            //    // free(png);
            //    catTexture = CreateTexture("./src/include/fancy.png");
            //    SetActiveTexture(catTexture);
            //    auto     texture  = GetActiveTexture();
            //    uint8_t *gaussian = texture.Convolve(Texture::Convolution::EdgeDetection);
            //    ImageViewer(gaussian, platform->colorBuffer.buffer, width, height, no_channels,
            //    platform->colorBuffer.width,
            //                platform->colorBuffer.height, platform->colorBuffer.noChannels, target_width,
            //                target_height);
            //    free(png);
            //    free(gaussian);
            /*  }
              else
                  std::cout << "OOOps .. Failed to load the given PNG";*/
            catTexture = CreateTexture("./src/include/fancy.png");
            if (!catTexture)
                std::cout << "Failed to create cat texture..";
            else
                std::cout << "Successfully created the cat texture..";

            fancyTexture = CreateTexture("./src/include/cat.png");
            if (!fancyTexture)
                std::cout << "Failed to create cat texture..";
            else
                std::cout << "Successfully created the cat texture..";
        }
        Device = GetRasteriserDevice();
        // Device->Context.SetRasteriserMode(RenderDevice::RasteriserMode::NONE_CULL);
        // Device->Context.SetMergeMode(RenderDevice::MergeMode::BLEND_MODE);
        // Device->Context.SetMergeMode(RenderDevice::MergeMode::TEXTURE_MODE);
        Device->Context.SetMergeMode(RenderDevice::MergeMode::TEXTURE_BLENDING_MODE);
    }

    // ClearColor(0x00, 0xFF, 0x00);
    FastClearColor(0xC0, 0x00, 0xC0, 0x80);
    /*float          aspect_ratio = platform->width * 1.0f / platform->height;
    VertexAttrib2D v0           = {Vec2f(-0.5f, -0.5f), Vec2f(0.35, 0.35), Vec4f(1.0f, 0.0f, 0.0f, 0.25f)};
    VertexAttrib2D v1           = {Vec2f(0.5f, 0.5f), Vec2f(0.35, 0.65), Vec4f(1.0f, 0.0f, 0.0f, 0.5f)};
    VertexAttrib2D v2           = {Vec2f(0.0f, 1.5f), Vec2f(0.65, 0.65), Vec4f(1.0f, 0.0f, 0.0f, 0.75f)};
    VertexAttrib2D v3           = {Vec2f(1.0f / aspect_ratio, -1.0f), Vec2f(0.65, 0.35), Vec4f(1.0f, 0.0f,
    0.0f, 1.0f)};*/

    // SetActiveTexture(catTexture);
    // ClipSpace(v0, v1, v2);
    // SetActiveTexture(fancyTexture);
    // ClipSpace(v0, v2, v3);
    // Device->Draw(v0, v1);
    // Device->Draw(v0, v1, v2);

    static float time_count = 0;
    time_count += platform->deltaTime;
    // Vertex Transform
    VertexAttrib2D v0              = {Vec2f(100, 100), Vec2f(0.35, 0.35), Vec4f(1.0f, 0.0f, 0.0f, 0.1f)};
    VertexAttrib2D v1              = {Vec2f(100, 400), Vec2f(0.35, 0.65), Vec4f(0.0f, 1.0f, 0.0f, 0.5f)};
    VertexAttrib2D v2              = {Vec2f(400, 400), Vec2f(0.65, 0.65), Vec4f(0.0f, 0.0f, 1.0f, 0.75f)};
    VertexAttrib2D v3              = {Vec2f(400, 100), Vec2f(0.65, 0.35), Vec4f(1.0f, 1.0f, 0.0f, 1.0f)};

    Mat4f          TransformMatrix = orthoProjection(0, platform->width, 0, platform->height, 0.0f, 1.0f);

    Device->Context.SetTransformMatrix(TransformMatrix.translate(Vec3f(250, 0, 0))
                                           .translate(Vec3f(250, 250, 0))
                                           .rotateZ(time_count / 2.0f)
                                           .translate(Vec3f(-250, -250, 0)));
    SetActiveTexture(catTexture);
    Device->Draw(v0, v1, v2);
    SetActiveTexture(fancyTexture);
    Device->Draw(v0, v2, v3);

    platform->SwapBuffer();
}