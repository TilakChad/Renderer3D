#include "./include/platform.h"
#include "./include/rasteriser.h"
#include "./include/raytracer.h"

#include "./image/PNGLoader.h"
#include "./include/geometry.hpp"
#include "./maths/vec.hpp"

#include <fstream>

static uint32_t                         catTexture;
static uint32_t                         fancyTexture;
static RenderDevice                    *Device = nullptr;
static Mat4f                            TransformMatrix;
int32_t                                 inc = 1;
static uint32_t                         importedTexture;

std::vector<Pipeline3D::VertexAttrib3D> GeoVertices;
std::vector<uint32_t>                   GeoVerticesIndex;

// Leave ray tracer for now and continue on rasteriser for sometime
// TODO : fragment shader emulation

Vec3f   cameraPosition = Vec3f();
float   yaw            = 0;
float   pitch          = 0;
int32_t xPos, yPos;

Vec3f   getFrontVector(Platform *platform)
{
    auto            delta_xPos     = platform->Mouse.xpos - xPos;
    auto            delta_yPos     = platform->Mouse.ypos - yPos;

    constexpr float angle_constant = 1.0f / 500;
    pitch                          = pitch - delta_yPos * angle_constant;
    yaw                            = yaw + delta_xPos * angle_constant;

    if (pitch > 1.55334f)
        pitch = 1.55334f;
    else if (pitch < -1.55334f)
        pitch = -1.55334f;

    auto frontVector = Vec3f();
    frontVector.x    = std::sin(yaw) * std::cos(pitch);
    frontVector.y    = std::sin(pitch);
    frontVector.z    = std::cos(pitch) * -std::cos(yaw);

    xPos             = platform->Mouse.xpos;
    yPos             = platform->Mouse.ypos;

    // std::cout << "Delta xpos and ypos are : " << delta_xPos << " " << delta_yPos << std::endl;
    constexpr float move_constant = 5.0f;
    auto            fVector       = frontVector.unit();

    if (platform->bKeyPressed(Keys::W))
        cameraPosition = cameraPosition + move_constant * platform->deltaTime * fVector;
    else if (platform->bKeyPressed(Keys::S))
        cameraPosition = cameraPosition - move_constant * platform->deltaTime * fVector;
    return fVector;
}

void RendererMainLoop(Platform *platform)
{

    if (platform->bFirst)
    {
        ClearColor(0x87, 0xCE, 0xEB);
        // Initialize the rendering device
        platform->bFirst = false;

        { // Rasterisation

            importedTexture = CreateTexture("../Blender/SmartIUV.png");
            if (!importedTexture)
                std::cout << "Failed to create imported texture..";
            else
                std::cout << "Successfully created the imported texture..";

            Device = GetRasteriserDevice();
            // Device->Context.SetRasteriserMode(RenderDevice::RasteriserMode::NONE_CULL);
            // Device->Context.SetMergeMode(RenderDevice::MergeMode::BLEND_MODE);
            Device->Context.SetMergeMode(RenderDevice::MergeMode::TEXTURE_MODE);
            // Device->Context.SetMergeMode(RenderDevice::MergeMode::TEXTURE_BLENDING_MODE);

            Object3D model("../Blender/macube.obj");

            GeoVertices.clear();
            GeoVerticesIndex.clear();
            
            model.LoadGeometry(GeoVertices, GeoVerticesIndex);
            
            std::cout << "Total number of vertices loaded were : " << GeoVertices.size() << std::endl;
            std::cout << "Total number of indices loaded were : " << GeoVerticesIndex.size() << std::endl;
            platform->SetOpacity(1.0f);
        }
    }
    // Rasterisation
    // ClearColor(0x00, 0xFF, 0x00);
    FastClearColor(0xC0, 0xC0, 0xC0, 0xC0);
    SetActiveTexture(importedTexture);

    using namespace Pipeline3D;
    Mat4f transform = Perspective(platform->width * 1.0f / platform->height, 0.4f / 3 * 3.141592f, 0.3f);
    auto  model     = Mat4f(1.0f)
                     .translate(Vec3f(0.0f, 0.0f, -6.5f))
                     .translate(Vec3f(0.0f, 0.0f, 0.0f))
                     .scale(Vec3f(0.5f, 0.5f, 0.5f));

    ClearDepthBuffer();
    SetActiveTexture(importedTexture);
    auto lookMatrix = lookAtMatrix(cameraPosition, cameraPosition + getFrontVector(platform), Vec3f(0.0f, 1.0f, 0.0f));
    transform       = transform * lookMatrix * model;
    Pipeline3D::Draw(GeoVertices, GeoVerticesIndex, transform);
    platform->SwapBuffer();
}