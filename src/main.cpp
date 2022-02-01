#include "./include/geometry.hpp"
#include "./include/platform.h"
#include "./include/rasteriser.h"
#include "./include/raytracer.h"
#include "./include/render.h"

#include "./image/PNGLoader.h"
#include "./maths/vec.hpp"

#include "./utils/memalloc.h"
#include "./utils/parallel_render.h"
#include "./utils/shapes.h"
#include "./utils/thread_pool.h"

// Lets now work on lighting

// Start of the optimization phase of rasterisation
static uint32_t                         catTexture;
static uint32_t                         fancyTexture;
static RenderDevice                    *Device = nullptr;

static uint32_t                         importedTexture;

std::vector<Pipeline3D::VertexAttrib3D> Vertices;
std::vector<uint32_t>                   Indices;

RenderList                              Renderables{};
BackgroundTexture                       bckg;
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

Vec3f          cameraPosition = Vec3f();
float          yaw            = 0;
float          pitch          = 0;
int32_t        xPos, yPos;

static RLights current_light{};

RLights        get_light_source()
{
    return current_light;
}

Vec3f get_camera_position()
{
    return cameraPosition;
}

Vec3f getFrontVector(Platform *platform)
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

    auto frontVector              = Vec3f();
    frontVector.x                 = std::sin(yaw) * std::cos(pitch);
    frontVector.y                 = std::sin(pitch);
    frontVector.z                 = std::cos(pitch) * -std::cos(yaw);

    xPos                          = platform->Mouse.xpos;
    yPos                          = platform->Mouse.ypos;

    constexpr float move_constant = 50.0f;
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
            Object3D model("../Blender/macube.obj");
            Object3D model2("../Blender/unwrappedorder.obj");
            // Object3D model("../Blender/bagpack/backpack.obj");

            std::vector<Pipeline3D::VertexAttrib3D> vertices{};
            std::vector<uint32_t>                   indices{};

            model.LoadGeometry(vertices, indices);
            Renderables.AddRenderable(RenderInfo(std::move(vertices), std::move(indices),
                                                  RenderDevice::MergeMode::COLOR_MODE,
                                                  model.Materials.at(0).texture_id));
            /*model2.LoadGeometry(GeoSphVertices, GeoSphVerticesIndex);*/
            // platform->SetOpacity(1.0f);
            // fancyTexture = model.Materials.at(0).texture_id;
            //  catTexture   = model2.Materials.at(0).texture_id;
            for (int i = 0; i < 8; ++i)
            {
                resource.push_back(MonotonicMemoryResource{std::malloc(1024 * 1024), 1024 * 1024});
            }
            for (int i = 0; i < 8; ++i)
            {
                MemAllocator.push_back(MemAlloc<Pipeline3D::VertexAttrib3D>(&resource.at(i)));
            }
            parallel_renderer = Parallel::ParallelRenderer(platform->colorBuffer.width, platform->colorBuffer.height);

            // Create a checked texture
            Pipeline3D::VertexAttrib3D v0, v1, v2, v3;
            v0.Position = Vec4f(-5.0f, 0.0f, 5.0f, 1.0f);
            v0.Color    = Vec4f(1.0f, 0.0f, 0.0f, 0.0f);
            v0.TexCoord = Vec2f(0.0f, 0.0f);
            v1.Position = Vec4f(5.0f, 0.0f, 5.0f, 1.0f);
            v1.Color    = Vec4f(0.0f, 1.0f, 0.0f, 0.0f);
            v1.TexCoord = Vec2f(1.0f, 0.0f);
            v2.Position = Vec4f(5.0f, 0.0f, -5.0f, 1.0f);
            v2.Color    = Vec4f(0.0f, 0.0f, 1.0f, 0.0f);
            v2.TexCoord = Vec2f(1.0f, 1.0f);
            v3.Position = Vec4f(-5.0f, 0.0f, -5.0f, 1.0f);
            v3.Color    = Vec4f(1.0f, 1.0f, 0.0f, 0.0f);
            v3.TexCoord = Vec2f(0.0f, 1.0f);

            Vertices.insert(Vertices.end(), {v0, v1, v2, v3});
            Indices.insert(Indices.end(), {1, 2, 0, 0, 2, 3});

            // Allocate a texture memory and write all the pixels yourself .. less do it
            Texture texture;
            texture.width  = 100;
            texture.height = 100;

            // Each texture with only 1 byte of data per pixel
            texture.bit_depth = 8;
            texture.channels  = 1;
            constexpr int dim = 10;
            texture.raw_data  = new uint8_t[texture.width * texture.height];

            for (int h = 0; h < texture.height; ++h)
            {
                int a = h / dim;
                for (int w = 0; w < texture.width; ++w)
                {
                    int b = w / dim;
                    if ((a + b) % 2 == 0)
                        texture.raw_data[h * texture.width + w] = 0xFF;
                    else
                        texture.raw_data[h * texture.width + w] = 0x00;
                }
            }
            fancyTexture = CreateTextureFromData(texture);
            SetActiveTexture(fancyTexture);

            // Lets try generating cylinder from nothingness - aka doing magic
            // A little flashback to parametric equations (Parametric equations are :love:)
            // To create a cylinder standing on xz axis, we have

            // x = r * cos(theta); theta is the angle between radial vector and x axis
            // z = r * sin(theta);
            // y = h (taken in steps)

            // Renderables.AddRenderable(RenderInfo(std::move(Vertices), std::move(Indices),
            //                                   RenderDevice::MergeMode::TEXTURE_MODE, fancyTexture));

            // Renderables.AddRenderable(Shape::Cylinder::offload(1.0f, 2.0f));
            // Renderables.AddRenderable(Shape::Sphere::offload(1.0f));

            current_light = RLights{
                .position = Vec4f(0.0f, 0.0f, 5.0f, 1.0f), .color = Vec4f(1.0f, 215.0f/255.0f, 0x00, 0x00), .intensity = 1.0f};
        }
        bckg.CreateBackgroundTexture("../img103.png");
        bckg.SampleForCurrentFrameBuffer(platform, false);
        cameraPosition = Vec3f(0.0f, 0.0f, 6.0f);
    }
    else if (platform->bSizeChanged)
    {
        platform->bSizeChanged = false;
        parallel_renderer      = Parallel::ParallelRenderer(platform->colorBuffer.width, platform->colorBuffer.height);
        bckg.SampleForCurrentFrameBuffer(platform, true);
    }
    static float time = 0.0f;
    // rotate light 
    // current_light.position = Vec4f(2 * cos(time / 5.0f), 0.0f, 2 * sin(time / 5.0f), 1.0f);
    // cameraPosition.z += 0.01f;
    // current_light.position.z -= 0.001f;
    time += platform->deltaTime;
    // RenderBackground(bckg);
    FastClearColor(0x10, 0x10, 0x10, 0x00);
    using namespace Pipeline3D;
    Mat4f transform = Perspective(platform->width * 1.0f / platform->height, 0.4f / 3 * 3.141592f, 0.3f, 20.0f);
    ClearDepthBuffer();
    auto model      = Mat4f(1.0f).rotateY(time); //.rotateX(time / 2.0f);
    auto lookMatrix = lookAtMatrix(cameraPosition, cameraPosition + getFrontVector(platform), Vec3f(0.0f, 1.0f, 0.0f));
    // auto lookMatrix = lookAtMatrix(cameraPosition, cameraPosition + Vec3f(0.0f, 0.0f, -1.0f), Vec3f(0.0f, 1.0f,
    // 0.0f));

    // seperate the model matrix from here to other space, since we also ned to interpolate the vertex position like
    // other things in the screen space to calculate other effects so basically yes, its all to calculate fragpos Lets
    // implement flat shading for now, instead of per pixel lighting .. we will come back to it
    for (auto &renderable : Renderables.Renderables)
    {
        renderable.scene_transform = transform * lookMatrix;
        renderable.model_transform = model;
    }

    parallel_renderer.AlternativeParallelRenderablePipeline(thread_pool, Renderables, MemAllocator);
    platform->SwapBuffer();
}

Parallel::ParallelRenderer &get_current_parallel_renderer()
{
    return parallel_renderer;
}
