#include "./include/platform.h"
#include "./include/rasteriser.h"
#include "./include/raytracer.h"

#include "./image/PNGLoader.h"
#include "./include/geometry.hpp"
#include "./maths/vec.hpp"

#include "./utils/memalloc.h"
#include "./utils/parallel_render.h"
#include "./utils/thread_pool.h"

#include <fstream>

// Start of the optimization phase of rasterisation
static uint32_t      catTexture;
static uint32_t      fancyTexture;
static RenderDevice *Device = nullptr;
// static Mat4f                            TransformMatrix;
// static SIMD::Mat4ss                     TransformMatrix;
int32_t                                 inc = 1;
static uint32_t                         importedTexture;

std::vector<Pipeline3D::VertexAttrib3D> GeoVertices;
std::vector<uint32_t>                   GeoVerticesIndex;

// System wide resources
// Thread local didn't solve any .. only created problems
// Since these thread local things get uninitialized, they should probably be initialized during the creation of thread
// pool Now going to make MemAlloc thread safe
std::vector<MemAlloc<Pipeline3D::VertexAttrib3D>> MemAllocator;
std::vector<MonotonicMemoryResource>              resource;
// making thread_pool thread_local is thread bomb .. insta OS stop responding
// ThreadPool                                        thread_pool{};
Alternative::ThreadPool    thread_pool{};
Parallel::ParallelRenderer parallel_renderer;
// Leave ray tracer for now and continue on rasteriser for sometime
// TODO : fragment  shader emulation
// TODO : Vectorize rasterisation code and matrix operations

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
            Device = GetRasteriserDevice();
            // Device->Context.SetRasteriserMode(RenderDevice::RasteriserMode::NONE_CULL);
            // Device->Context.SetMergeMode(RenderDevice::MergeMode::BLEND_MODE);
            // Device->Context.SetMergeMode(RenderDevice::MergeMode::COLOR_MODE);
            Device->Context.SetMergeMode(RenderDevice::MergeMode::TEXTURE_MODE);
            // Device->Context.SetMergeMode(RenderDevice::MergeMode::TEXTURE_BLENDING_MODE);
            // Device->Context.SetMergeMode(RenderDevice::MergeMode::NOTHING);
            Object3D model("../Blender/unwrappedorder.obj");
            // Object3D model("../Blender/macube.obj");
            // Object3D model("../Blender/bagpack/backpack.obj");

            GeoVertices.clear();
            GeoVerticesIndex.clear();

            model.LoadGeometry(GeoVertices, GeoVerticesIndex);

            std::cout << "Total number of vertices loaded were : " << GeoVertices.size() << std::endl;
            std::cout << "Total number of indices loaded were : " << GeoVerticesIndex.size() << std::endl;
            platform->SetOpacity(1.0f);
            SetActiveTexture(model.Materials.at(0).texture_id);

            // Initialize the monotonic buffer resource here
            // resource          = MonotonicMemoryResource{std::malloc(256*1024), 1024*256};
            //  MemAllocator      = MemAlloc<Pipeline3D::VertexAttrib3D>(&resource);
            for (int i = 0; i < 8; ++i)
            {
                resource.push_back(MonotonicMemoryResource{std::malloc(1024 * 1024), 1024 * 1024});
            }
            for (int i = 0; i < 8; ++i)
            {
                MemAllocator.push_back(MemAlloc<Pipeline3D::VertexAttrib3D>(&resource.at(i)));
            }
            parallel_renderer = Parallel::ParallelRenderer(platform->colorBuffer.width, platform->colorBuffer.height);
        }
    }
    else if (platform->bSizeChanged)
    {
        platform->bSizeChanged = false;
        parallel_renderer      = Parallel::ParallelRenderer(platform->colorBuffer.width, platform->colorBuffer.height);
    }
    // Rasterisation
    static float time = 0.0f;
    time += platform->deltaTime;
    // ClearColor(0x00, 0xFF, 0x00);
    FastClearColor(0xC0, 0xC0, 0xC0, 0xC0);
    using namespace Pipeline3D;
    Mat4f transform = Perspective(platform->width * 1.0f / platform->height, 0.4f / 3 * 3.141592f, 0.3f);
    // auto transform = SIMD::Perspective(platform->width * 1.0f / platform->height, 0.4f / 3 * 3.141592f, 0.3f);
    //      .scale(Vec3f(0.5f, 0.5f, 0.5f));
    ClearDepthBuffer();
    /* transform =
        transform * SIMD::lookAtMatrix(cameraPosition, cameraPosition + getFrontVector(platform), Vec3f(0.0f, 1.0f,
        0.0f));
    */ // transform = transform.translate(Vec3f(0.0f, 0.0f, -4.5f));

    // .rotateY(time);
    auto model      = Mat4f(1.0f).translate(Vec3f(0.0f, 0.0f, -4.5f)).rotateY(time);
    auto lookMatrix = lookAtMatrix(cameraPosition, cameraPosition + getFrontVector(platform), Vec3f(0.0f, 1.0f, 0.0f));
    transform       = transform * lookMatrix * model;
    //  auto transform = Mat4(1.0f);

    // Pipeline3D::Draw(GeoVertices, GeoVerticesIndex, transform, MemAllocator);
    parallel_renderer.AlternativeParallelPipeline(thread_pool, GeoVertices, GeoVerticesIndex, transform, MemAllocator);
    // parallel_renderer.ParallelPipeline(thread_pool, GeoVertices, GeoVerticesIndex, transform, MemAllocator);
    platform->SwapBuffer();
}

// ThreadPool &get_current_thread_pool()
//{
//     return thread_pool;
// }

Parallel::ParallelRenderer &get_current_parallel_renderer()
{
    return parallel_renderer;
}
