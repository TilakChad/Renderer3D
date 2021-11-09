#include "./include/platform.h"
#include "./include/rasteriser.h"

#include "./image/PNGLoader.h"
#include "./maths/vec.hpp"

static uint32_t      catTexture;
static uint32_t      fancyTexture;
static RenderDevice *Device = nullptr;
static Mat4f         TransformMatrix;
int32_t              inc = 1;
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
            catTexture = CreateTexture("./src/include/checker.png");
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
        Device->Context.SetRasteriserMode(RenderDevice::RasteriserMode::NONE_CULL);
        // Device->Context.SetMergeMode(RenderDevice::MergeMode::BLEND_MODE);
        Device->Context.SetMergeMode(RenderDevice::MergeMode::TEXTURE_MODE);
        // Device->Context.SetMergeMode(RenderDevice::MergeMode::TEXTURE_BLENDING_MODE);
        TransformMatrix = OrthoProjection(0, platform->width, 0, platform->height, 0.0f, 1.0f);
        Device->Context.SetTransformMatrix(TransformMatrix.translate(Vec3f(250, 0, 0))
                                               .translate(Vec3f(250, 250, 0))
                                               .rotateZ(36.0f / 2.0f)
                                               .translate(Vec3f(-250, -250, 0)));

        // Mat4f transform = PerspectiveAlt(platform->width * 1.0f / platform->height, 0.5f / 3 * 3.141592f, 0.3f);
        // std::cout << "\nMatrix is : " << transform << std::endl;

        // auto Vc = Vec4f(0.0f, 0.35f, -2.9f, 1.0f);
        // auto Vb = Vec4f(0.5f, 0.0f, -1.5f, 1.0f);
        // auto mid = (Vc + Vb) * (1 / 2.0f);
        // std::cout << "\nMid is : " << mid << std::endl;
        // std::cout << "\nVC is : " << transform * Vc << std::endl;
        // std::cout << "\n\nVB is : " << transform * Vb << std::endl;
        // std::cout << "\nMid transformed is : " << transform * mid << std::endl;
    }

    // ClearColor(0x00, 0xFF, 0x00);
    FastClearColor(0xC0, 0xC0, 0xC0, 0xC0);
    /*float          aspect_ratio = platform->width * 1.0f / platform->height;
    Pipeline2D::VertexAttrib2D v0           = {Vec2f(-0.5f, -0.5f), Vec2f(0.35, 0.35), Vec4f(1.0f, 0.0f, 0.0f, 0.25f)};
    Pipeline2D::VertexAttrib2D v1           = {Vec2f(0.5f, 0.5f), Vec2f(0.35, 0.65), Vec4f(1.0f, 0.0f, 0.0f, 0.5f)};
    Pipeline2D::VertexAttrib2D v2           = {Vec2f(0.0f, 1.5f), Vec2f(0.65, 0.65), Vec4f(1.0f, 0.0f, 0.0f, 0.75f)};
    Pipeline2D::VertexAttrib2D v3           = {Vec2f(1.0f / aspect_ratio, -1.0f), Vec2f(0.65, 0.35), Vec4f(1.0f, 0.0f,
    0.0f, 1.0f)};*/

    SetActiveTexture(catTexture);
    // ClipSpace(v0, v1, v2);
    // SetActiveTexture(fancyTexture);
    // ClipSpace(v0, v2, v3);
    // Device->Draw(v0, v1);
    // Device->Draw(v0, v1, v2);

    static float time_count = 0;
    time_count += inc * platform->deltaTime;
    //// Vertex Transform
    Pipeline2D::VertexAttrib2D v0 = {Vec2f(100, 100), Vec2f(0.35, 0.35), Vec4f(1.0f, 0.0f, 0.0f, 0.1f)};
    Pipeline2D::VertexAttrib2D v1 = {Vec2f(100, 400), Vec2f(0.35, 0.65), Vec4f(0.0f, 1.0f, 0.0f, 0.5f)};
    Pipeline2D::VertexAttrib2D v2 = {Vec2f(800, 400), Vec2f(0.65, 0.65), Vec4f(0.0f, 0.0f, 1.0f, 0.75f)};
    Pipeline2D::VertexAttrib2D v3 = {Vec2f(400, 100), Vec2f(0.65, 0.35), Vec4f(1.0f, 1.0f, 0.0f, 1.0f)};

    // Mat4f          TransformMatrix = orthoProjection(0, platform->width, 0, platform->height, 0.0f, 1.0f);

    // Device->Context.SetTransformMatrix(TransformMatrix.translate(Vec3f(250, 0, 0))
    //                                       .translate(Vec3f(250, 250, 0))
    //                                       .rotateZ(time_count / 2.0f)
    //                                       .translate(Vec3f(-250, -250, 0)));
    // SetActiveTexture(catTexture);
    // Device->Draw(v0, v1, v2);
    // SetActiveTexture(fancyTexture);
    // if (time_count > 9.0f)
    //    inc = -1;
    // if (time_count < 0.3f)
    //    inc = 1;
    // Device->Draw(v0, v2, v3);
    {
        // 3D pipeline check stuffs
        // Mat4f transform = Perspective(platform->width * 1.0f / platform->height, 1.0 / 3 * 3.141592f, 0.3f)
        //                      .translate(Vec3f(0.0f, 0.0f, -time_count));
        // Pipeline3D::VertexAttrib3D v0 = {Vec4f(-0.5f, -0.5f, 0.0f, 1.0f), Vec2f(0), Vec4f(1.0f, 0.0f, 0.0f, 1.0f)};
        // Pipeline3D::VertexAttrib3D v1 = {Vec4f(0.5f, -0.5f, 0.0f, 1.0f), Vec2f(0), Vec4f(0.0f, 1.0f, 0.0f, 1.0f)};
        // Pipeline3D::VertexAttrib3D v2 = {Vec4f(0.5f, 0.5f, 0.0f, 1.0f), Vec2f(0), Vec4f(0.0f, 0.0f, 1.0f, 1.0f)};
        // Pipeline3D::Draw(v0, v1, v2, transform);
    } {
        using namespace Pipeline3D;
        // Mat4f transform = Perspective(platform->width * 1.0f / platform->height, 0.5f / 3 * 3.141592f,
        //                              0.3f); //.translate(Vec3f(0.0f, 0.0f, -1.5f));
        //// .rotateY(time_count);
        Mat4f transform = Perspective(platform->width * 1.0f / platform->height, 0.6f / 3 * 3.141592f, 0.3f)
                              .translate(Vec3f(0.0f, 0.25f, -4.5f))
                              .rotateX(time_count * 6.0f)
                              .rotateY(time_count)
                              .translate(Vec3f(0.0f, -0.5f, -0.5f))
                              .scale(Vec3f(1.0f, 1.0f, 5.0f));

        ClearDepthBuffer();
        VertexAttrib3D c0  = {Vec4f(-0.5f, -0.5f, -0.5f, 1.0f), Vec2f(0.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c1  = {Vec4f(0.5f, -0.5f, -0.5f, 1.0f), Vec2f(1.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c2  = {Vec4f(0.5f, 0.5f, -0.5f, 1.0f), Vec2f(1.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c3  = {Vec4f(0.5f, 0.5f, -0.5f, 1.0f), Vec2f(1.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c4  = {Vec4f(-0.5f, 0.5f, -0.5f, 1.0f), Vec2f(0.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c5  = {Vec4f(-0.5f, -0.5f, -0.5f, 1.0f), Vec2f(0.0f, 0.0f), Vec4f(0)};

        VertexAttrib3D c6  = {Vec4f(-0.5f, -0.5f, 0.5f, 1.0f), Vec2f(0.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c7  = {Vec4f(0.5f, -0.5f, 0.5f, 1.0f), Vec2f(1.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c8  = {Vec4f(0.5f, 0.5f, 0.5f, 1.0f), Vec2f(1.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c9  = {Vec4f(0.5f, 0.5f, 0.5f, 1.0f), Vec2f(1.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c10 = {Vec4f(-0.5f, 0.5f, 0.5f, 1.0f), Vec2f(0.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c11 = {Vec4f(-0.5f, -0.5f, 0.5f, 1.0f), Vec2f(0.0f, 0.0f), Vec4f(0)};

        VertexAttrib3D c12 = {Vec4f(-0.5f, 0.5f, 0.5f, 1.0f), Vec2f(1.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c13 = {Vec4f(-0.5f, 0.5f, -0.5f, 1.0f), Vec2f(1.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c14 = {Vec4f(-0.5f, -0.5f, -0.5f, 1.0f), Vec2f(0.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c15 = {Vec4f(-0.5f, -0.5f, -0.5f, 1.0f), Vec2f(0.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c16 = {Vec4f(-0.5f, -0.5f, 0.5f, 1.0f), Vec2f(0.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c17 = {Vec4f(-0.5f, 0.5f, 0.5f, 1.0f), Vec2f(1.0f, 0.0f), Vec4f(0)};

        VertexAttrib3D c18 = {Vec4f(0.5f, 0.5f, 0.5f, 1.0f), Vec2f(1.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c19 = {Vec4f(0.5f, 0.5f, -0.5f, 1.0f), Vec2f(1.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c20 = {Vec4f(0.5f, -0.5f, -0.5f, 1.0f), Vec2f(0.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c21 = {Vec4f(0.5f, -0.5f, -0.5f, 1.0f), Vec2f(0.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c22 = {Vec4f(0.5f, -0.5f, 0.5f, 1.0f), Vec2f(0.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c23 = {Vec4f(0.5f, 0.5f, 0.5f, 1.0f), Vec2f(1.0f, 0.0f), Vec4f(0)};

        VertexAttrib3D c24 = {Vec4f(-0.5f, -0.5f, -0.5f, 1.0f), Vec2f(0.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c25 = {Vec4f(0.5f, -0.5f, -0.5f, 1.0f), Vec2f(1.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c26 = {Vec4f(0.5f, -0.5f, 0.5f, 1.0f), Vec2f(1.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c27 = {Vec4f(0.5f, -0.5f, 0.5f, 1.0f), Vec2f(1.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c28 = {Vec4f(-0.5f, -0.5f, 0.5f, 1.0f), Vec2f(0.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c29 = {Vec4f(-0.5f, -0.5f, -0.5f, 1.0f), Vec2f(0.0f, 1.0f), Vec4f(0)};

        VertexAttrib3D c30 = {Vec4f(-0.5f, 0.5f, -0.5f, 1.0f), Vec2f(0.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c31 = {Vec4f(0.5f, 0.5f, -0.5f, 1.0f), Vec2f(1.0f, 1.0f), Vec4f(0)};
        VertexAttrib3D c32 = {Vec4f(0.5f, 0.5f, 0.5f, 1.0f), Vec2f(1.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c33 = {Vec4f(0.5f, 0.5f, 0.5f, 1.0f), Vec2f(1.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c34 = {Vec4f(-0.5f, 0.5f, 0.5f, 1.0f), Vec2f(0.0f, 0.0f), Vec4f(0)};
        VertexAttrib3D c35 = {Vec4f(-0.5f, 0.5f, -0.5f, 1.0f), Vec2f(0.0f, 1.0f), Vec4f(0)};

        auto z0 = VertexAttrib3D(Vec4f(-0.5f, 0.0f, -1.5f, 1.0f), Vec2f(0.0f, 1.0f), Vec4f(1.0f, 0.0f, 0.0f, 1.0f));
        auto z1 = VertexAttrib3D(Vec4f(0.5f, 0.0f, -1.5f, 1.0f), Vec2f(1.0f, 0.0f), Vec4f(0.0f, 1.0f, 0.0f, 1.0f));
        auto z2 = VertexAttrib3D(Vec4f(0.0f, 0.35f, -2.9f, 1.0f), Vec2f(1.0f, 1.0f), Vec4f(0.0f, 0.0f, 1.0f, 1.0f));

        Pipeline3D::Draw(c0, c1, c2, transform);
        Pipeline3D::Draw(c3, c4, c5, transform);

        Pipeline3D::Draw(c6, c7, c8, transform);
        Pipeline3D::Draw(c9, c10, c11, transform);

        Pipeline3D::Draw(c12, c13, c14, transform);
        Pipeline3D::Draw(c15, c16, c17, transform);

        Pipeline3D::Draw(c18, c19, c20, transform);
        Pipeline3D::Draw(c21, c22, c23, transform);

        Pipeline3D::Draw(c24, c25, c26, transform);
        Pipeline3D::Draw(c27, c28, c29, transform);

        Pipeline3D::Draw(c30, c31, c32, transform);
        Pipeline3D::Draw(c33, c34, c35, transform);

        // Pipeline3D::Draw(z0, z1, z2, transform);
    }
    platform->SwapBuffer();
}