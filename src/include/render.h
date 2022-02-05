#pragma once

#include "./rasteriser.h"
#include <vector>
#include <array>

// Helpers to initiate or modify rendering operations
// Render structs

class RenderInfo
{

    // For types that support move operations
  public:
    uint32_t                textureID;
    RenderDevice::MergeMode merge_mode;
    // These two aren't taken by constructor .. need to initialize them manually
    Mat4f                                   scene_transform;
    Mat4f                                   model_transform;
    std::vector<uint32_t>                   indices;
    std::vector<Pipeline3D::VertexAttrib3D> vertices;
    // Optionally material to be used with it
    // Caution : This constructor will force move the vector out of the current container
    // Don't reuse the container after this
    // This is not rust, so C++ compiler is helpless against such bugs.
    RenderInfo(std::vector<Pipeline3D::VertexAttrib3D> &&vertexList, std::vector<uint32_t> &&indexList,
               RenderDevice::MergeMode output_merge_mode, uint32_t texture_id_for_texture)
        : vertices{std::move(vertexList)}, indices{std::move(indexList)},
          merge_mode{output_merge_mode}, textureID{texture_id_for_texture}
    {
    }
    RenderInfo(RenderInfo const &render_info) = delete;
    RenderInfo &operator=(RenderInfo const &render_info) = delete;
    RenderInfo(RenderInfo &&render_info)                 = default;
    RenderInfo &operator=(RenderInfo &&render_info) = default;
};

class RenderList
{

  public:
    std::vector<RenderInfo> Renderables{};
    RenderList() = default;
    void AddRenderable(RenderInfo &&render_info)
    {
        Renderables.push_back(std::move(render_info));
    }
};

inline void RenderWorld(RenderList &renderables)
{
}

inline void ParallelRenderWorld(RenderList &renderables)
{
}

class SceneDescription
{
  public:
    RenderList           Renderables;
    std::vector<RLights> light_sources;
    BackgroundTexture    bckg_texture;
};

struct Bezier
{
    // define Bernstein polynomial first aka Bezier blending functions
    // afaik they are similiar to binomial coefficients
    constexpr static uint8_t N               = 10;
    std::array<std::array<uint64_t, N>, N> binoCoeff = {{}};

    constexpr Bezier()
    {
        binoCoeff[0][0] = 0; 
        binoCoeff[1][1] = 1; 
        binoCoeff[1][0] = 1;
        for (int i = 2; i < N; ++i)
        {
            binoCoeff[i][0] = 1;
            for (int k = 1; k <= i; ++k)
            {
                binoCoeff[i][k] = binoCoeff[i - 1][k] + binoCoeff[i - 1][k - 1];
            }
        }
    }

    constexpr Vec3f BezierBlending(std::vector<Vec3f> &BezierPoints, float t) const 
    {
        // nCk x^k (1-x)^(n-k) .. I guess this is it
        // calculate nCk using memoization technique
        // nCk + nCk-1 = n+1Cr => n-1Ck + n-1Ck-1 = nCk
        // Base case : 1C1 = 1 and nCn = 1
        // just blend the function now .. easy
        // Lol how to blend?
        auto n     = BezierPoints.size() - 1;
        auto point = Vec3f(0.0f, 0.0f, 0.0f);
        for (int k = 0; k <= n; ++k)
        {
            point = point + binoCoeff[n][k] * powf(t, k) * powf(1.0f - t, n - k) * BezierPoints[k];
        }
        return point;
    }
};

constexpr inline Bezier BezierBlender;