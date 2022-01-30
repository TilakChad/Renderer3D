#include "./parallel_render.h"

namespace Parallel
{
using namespace Pipeline3D;
static void Rasteriser(Pipeline3D::RasterInfo const &v0, Pipeline3D::RasterInfo const &v1,
                       Pipeline3D::RasterInfo const &v2, int32_t XMinBound, int32_t XMaxBound)
{
    Platform platform = GetCurrentPlatform();

    auto     Device   = GetRasteriserDevice();
    int32_t  minX     = vMax(vMin(v0.x, v1.x, v2.x), XMinBound);
    int32_t  maxX     = vMin(vMax(v0.x, v1.x, v2.x), XMaxBound);
    /*int      minX     = std::min(std::min(v0.x, v1.x, v2.x), XMaxBound);
    int      maxX     = std::min(std::max(v0.x, v1.x, v2.x), XMinBound);*/

    int minY = vMin(v0.y, v1.y, v2.y);
    int maxY = vMax(v0.y, v1.y, v2.y);

    // Assume vectors are in clockwise ordering
    Vec2  p0   = Vec2(v1.x, v1.y) - Vec2(v0.x, v0.y);
    Vec2  p1   = Vec2(v2.x, v2.y) - Vec2(v1.x, v1.y);
    Vec2  p2   = Vec2(v0.x, v0.y) - Vec2(v2.x, v2.y);

    Vec2  vec0 = Vec2(v0.x, v0.y);
    Vec2  vec1 = Vec2(v1.x, v1.y);
    Vec2  vec2 = Vec2(v2.x, v2.y);

    float area = Vec2<int>::Determinant(p1, p0);

    // Perspective depth interpolation, perspective correct texture interpolation ....
    Vec2 point = Vec2(minX, minY);
    // Start collecting points from here

    float a1_ = Vec2<int32_t>::Determinant(point - vec0, p0);
    float a2_ = Vec2<int32_t>::Determinant(point - vec1, p1);
    float a3_ = Vec2<int32_t>::Determinant(point - vec2, p2);

    // Read to process 4 pixels at a time
    SIMD::Vec4ss a1_vec  = SIMD::Vec4ss(a1_, a1_ + p0.y, a1_ + 2 * p0.y, a1_ + 3 * p0.y);
    SIMD::Vec4ss a2_vec  = SIMD::Vec4ss(a2_, a2_ + p1.y, a2_ + 2 * p1.y, a2_ + 3 * p1.y);
    SIMD::Vec4ss a3_vec  = SIMD::Vec4ss(a3_, a3_ + p2.y, a3_ + 2 * p2.y, a3_ + 3 * p2.y);

    SIMD::Vec4ss inc_a1  = SIMD::Vec4ss(4 * p0.y);
    SIMD::Vec4ss inc_a2  = SIMD::Vec4ss(4 * p1.y);
    SIMD::Vec4ss inc_a3  = SIMD::Vec4ss(4 * p2.y);

    SIMD::Vec4ss inc_a1_ = SIMD::Vec4ss(-p0.x);
    SIMD::Vec4ss inc_a2_ = SIMD::Vec4ss(-p1.x);
    SIMD::Vec4ss inc_a3_ = SIMD::Vec4ss(-p2.x);

    SIMD::Vec4ss a1, a2, a3;
    SIMD::Vec4ss zvec(v0.z / area, v1.z / area, v2.z / area, 0.0f);
    // zvec = zvec;
    // Now processing downwards
    auto          inv_w     = SIMD::Vec4ss(v0.inv_w / area, v1.inv_w / area, v2.inv_w / area, 0.0f);
    constexpr int hStepSize = 4;
    if (Device->Context.ActiveMergeMode == RenderDevice::MergeMode::COLOR_MODE)
    {
        for (size_t h = minY; h <= maxY; ++h)
        {
            a1 = a1_vec;
            a2 = a2_vec;
            a3 = a3_vec;

            for (size_t w = minX; w <= maxX; w += hStepSize)
            {
                auto mask = SIMD::Vec4ss::generate_mask(a1, a2, a3);
                if (mask > 0)
                {

                    size_t offset = (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width *
                                    platform.colorBuffer.noChannels;
                    uint8_t *off   = platform.colorBuffer.buffer + offset + w * platform.colorBuffer.noChannels;
                    uint8_t *mem   = off;
                    auto     depth = &platform.zBuffer.buffer[h * platform.zBuffer.width + w];
                    using namespace SIMD;
                    auto   zero = _mm_setzero_ps();
                    __m128 a    = _mm_unpackhi_ps(a2.vec, a1.vec);
                    __m128 b    = _mm_unpackhi_ps(zero, a3.vec);
                    __m128 c    = _mm_unpacklo_ps(a2.vec, a1.vec);
                    __m128 d    = _mm_unpacklo_ps(zero, a3.vec);

                    // Lol shuffle ni garna parxa tw
                    auto lvec1 = Vec4ss(_mm_movehl_ps(a, b));
                    auto lvec2 = Vec4ss(_mm_movelh_ps(b, a));

                    auto lvec3 = Vec4ss(_mm_movehl_ps(c, d));
                    auto lvec4 = Vec4ss(_mm_movelh_ps(d, c));

                    lvec1      = Vec4ss(_mm_shuffle_ps(lvec1.vec, lvec1.vec, _MM_SHUFFLE(2, 1, 3, 0)));
                    lvec2      = Vec4ss(_mm_shuffle_ps(lvec2.vec, lvec2.vec, _MM_SHUFFLE(2, 1, 3, 0)));
                    lvec3      = Vec4ss(_mm_shuffle_ps(lvec3.vec, lvec3.vec, _MM_SHUFFLE(2, 1, 3, 0)));
                    lvec4      = Vec4ss(_mm_shuffle_ps(lvec4.vec, lvec4.vec, _MM_SHUFFLE(2, 1, 3, 0)));
                    if (mask & 0x08)
                    {
                        // plot first pixel
                        // calculate z

                        float z        = lvec1.dot(zvec);
                        lvec1          = lvec1 * inv_w;
                        auto  bary_sum = lvec1.dot(Vec4ss(1.0f, 1.0f, 1.0f, 0.0f));

                        float a[4];
                        _mm_store_ps(a, lvec1.vec);
                        if (z < depth[0])
                        {
                            auto rgb = (a[3] * v0.color + a[2] * v1.color + a[1] * v2.color) * (1.0f / bary_sum);
                            // sample texture
                            depth[0] = z;
                            mem[0]   = rgb.z * 255;
                            mem[1]   = rgb.y * 255;
                            mem[2]   = rgb.x * 255;
                            mem[3]   = rgb.w * 255;
                        }
                    }

                    if (mask & 0x04)
                    {
                        // plot second pixel
                        mem            = off + 4;

                        float z        = lvec2.dot(zvec);
                        lvec2          = lvec2 * inv_w;
                        auto  bary_sum = lvec2.dot(Vec4ss(1.0f, 1.0f, 1.0f, 0.0f));

                        float a[4];
                        _mm_store_ps(a, lvec2.vec);

                        if (z < depth[1])
                        {
                            // sample texture
                            auto rgb = (a[3] * v0.color + a[2] * v1.color + a[1] * v2.color) * (1.0f / bary_sum);
                            depth[1] = z;
                            mem[0]   = rgb.z * 255;
                            mem[1]   = rgb.y * 255;
                            mem[2]   = rgb.x * 255;
                            mem[3]   = rgb.w * 255;
                        }
                    }
                    if (mask & 0x02)
                    {
                        // plot third pixel
                        mem            = off + 8;
                        float z        = lvec3.dot(zvec);
                        lvec3          = lvec3 * inv_w;
                        auto  bary_sum = lvec3.dot(Vec4ss(1.0f, 1.0f, 1.0f, 0.0f));

                        float a[4];
                        _mm_store_ps(a, lvec3.vec);

                        if (z < depth[2])
                        {
                            // sample texture
                            auto rgb = (a[3] * v0.color + a[2] * v1.color + a[1] * v2.color) * (1.0f / bary_sum);

                            depth[2] = z;
                            mem[0]   = rgb.z * 255;
                            mem[1]   = rgb.y * 255;
                            mem[2]   = rgb.x * 255;
                            mem[3]   = 0x00;
                        }
                    }
                    if (mask & 0x01)
                    {
                        // plot fourth pixel
                        mem            = off + 12;
                        float z        = lvec4.dot(zvec);
                        lvec4          = lvec4 * inv_w;
                        auto  bary_sum = lvec4.dot(Vec4ss(1.0f, 1.0f, 1.0f, 0.0f));

                        float a[4];
                        _mm_store_ps(a, lvec4.vec);

                        if (z < depth[3])
                        {
                            // sample texture
                            auto rgb = (a[3] * v0.color + a[2] * v1.color + a[1] * v2.color) * (1.0f / bary_sum);
                            depth[3] = z;
                            mem[0]   = rgb.z * 255;
                            mem[1]   = rgb.y * 255;
                            mem[2]   = rgb.x * 255;
                            mem[3]   = 0x00;
                        }
                    }
                }

                a1 = a1 + inc_a1;
                a2 = a2 + inc_a2;
                a3 = a3 + inc_a3;
            }
            a1_vec = a1_vec + inc_a1_;
            a2_vec = a2_vec + inc_a2_;
            a3_vec = a3_vec + inc_a3_;
        }
    }
    else
    {
        // auto inv_w   = SIMD::Vec4ss(v0.inv_w / area, v1.inv_w / area, v2.inv_w / area, 0.0f);
        auto texture = GetActiveTexture();
        for (size_t h = minY; h <= maxY; ++h)
        {
            a1 = a1_vec;
            a2 = a2_vec;
            a3 = a3_vec;

            for (size_t w = minX; w <= maxX; w += hStepSize)
            {
                auto mask = SIMD::Vec4ss::generate_mask(a1, a2, a3);
                if (mask > 0)
                {

                    size_t offset = (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width *
                                    platform.colorBuffer.noChannels;
                    uint8_t *off   = platform.colorBuffer.buffer + offset + w * platform.colorBuffer.noChannels;
                    uint8_t *mem   = off;
                    auto     depth = &platform.zBuffer.buffer[h * platform.zBuffer.width + w];
                    using namespace SIMD;
                    auto   zero = _mm_setzero_ps();
                    __m128 a    = _mm_unpackhi_ps(a2.vec, a1.vec);
                    __m128 b    = _mm_unpackhi_ps(zero, a3.vec);
                    __m128 c    = _mm_unpacklo_ps(a2.vec, a1.vec);
                    __m128 d    = _mm_unpacklo_ps(zero, a3.vec);

                    // Lol shuffle ni garna parxa tw
                    auto lvec1 = Vec4ss(_mm_movehl_ps(a, b));
                    auto lvec2 = Vec4ss(_mm_movelh_ps(b, a));

                    auto lvec3 = Vec4ss(_mm_movehl_ps(c, d));
                    auto lvec4 = Vec4ss(_mm_movelh_ps(d, c));

                    lvec1      = Vec4ss(_mm_shuffle_ps(lvec1.vec, lvec1.vec, _MM_SHUFFLE(2, 1, 3, 0)));
                    lvec2      = Vec4ss(_mm_shuffle_ps(lvec2.vec, lvec2.vec, _MM_SHUFFLE(2, 1, 3, 0)));
                    lvec3      = Vec4ss(_mm_shuffle_ps(lvec3.vec, lvec3.vec, _MM_SHUFFLE(2, 1, 3, 0)));
                    lvec4      = Vec4ss(_mm_shuffle_ps(lvec4.vec, lvec4.vec, _MM_SHUFFLE(2, 1, 3, 0)));

                    // Retrieve the uv co-ordinate of texture using the barycentric co-ordinate
                    // These branches and lookup aren't that expensive
                    // mtx locking is expensive
                    // Depth and uv could be calculated incrementally, but lets not work on that for now
                    if (mask & 0x08)
                    {
                        // plot first pixel
                        // calculate z

                        float z        = lvec1.dot(zvec);
                        lvec1          = lvec1 * inv_w;
                        auto  bary_sum = lvec1.dot(Vec4ss(1.0f, 1.0f, 1.0f, 0.0f));

                        float a[4];
                        _mm_store_ps(a, lvec1.vec);
                        ;
                        auto uv = (a[3] * v0.texCoord + a[2] * v1.texCoord + a[1] * v2.texCoord) * (1.0f / bary_sum);
                        if (z < depth[0])
                        {
                            // sample texture
                            auto rgb = texture.Sample(uv);
                            depth[0] = z;
                            mem[0]   = rgb.z;
                            mem[1]   = rgb.y;
                            mem[2]   = rgb.x;
                            mem[3]   = 0x00;
                        }
                    }

                    if (mask & 0x04)
                    {
                        // plot second pixel
                        mem            = off + 4;

                        float z        = lvec2.dot(zvec);
                        lvec2          = lvec2 * inv_w;
                        auto  bary_sum = lvec2.dot(Vec4ss(1.0f, 1.0f, 1.0f, 0.0f));

                        float a[4];
                        _mm_store_ps(a, lvec2.vec);

                        auto uv = (a[3] * v0.texCoord + a[2] * v1.texCoord + a[1] * v2.texCoord) * (1.0f / bary_sum);
                        if (z < depth[1])
                        {
                            // sample texture
                            auto rgb = texture.Sample(uv);
                            depth[1] = z;
                            mem[0]   = rgb.z;
                            mem[1]   = rgb.y;
                            mem[2]   = rgb.x;
                            mem[3]   = 0x00;
                        }
                    }
                    if (mask & 0x02)
                    {
                        // plot third pixel
                        mem            = off + 8;
                        float z        = lvec3.dot(zvec);
                        lvec3          = lvec3 * inv_w;
                        auto  bary_sum = lvec3.dot(Vec4ss(1.0f, 1.0f, 1.0f, 0.0f));

                        float a[4];
                        _mm_store_ps(a, lvec3.vec);

                        auto uv = (a[3] * v0.texCoord + a[2] * v1.texCoord + a[1] * v2.texCoord) * (1.0f / bary_sum);
                        if (z < depth[2])
                        {
                            // sample texture
                            auto rgb = texture.Sample(uv);
                            depth[2] = z;
                            mem[0]   = rgb.z;
                            mem[1]   = rgb.y;
                            mem[2]   = rgb.x;
                            mem[3]   = 0x00;
                        }
                    }
                    if (mask & 0x01)
                    {
                        // plot fourth pixel
                        mem            = off + 12;
                        float z        = lvec4.dot(zvec);
                        lvec4          = lvec4 * inv_w;
                        auto  bary_sum = lvec4.dot(Vec4ss(1.0f, 1.0f, 1.0f, 0.0f));

                        float a[4];
                        _mm_store_ps(a, lvec4.vec);

                        auto uv = (a[3] * v0.texCoord + a[2] * v1.texCoord + a[1] * v2.texCoord) * (1.0f / bary_sum);
                        if (z < depth[3])
                        {
                            // sample texture
                            auto rgb = texture.Sample(uv);
                            depth[3] = z;
                            mem[0]   = rgb.z;
                            mem[1]   = rgb.y;
                            mem[2]   = rgb.x;
                            mem[3]   = 0x00;
                        }
                    }
                }

                a1 = a1 + inc_a1;
                a2 = a2 + inc_a2;
                a3 = a3 + inc_a3;
            }
            a1_vec = a1_vec + inc_a1_;
            a2_vec = a2_vec + inc_a2_;
            a3_vec = a3_vec + inc_a3_;
        }
    }
}

static void ScreenSpace(VertexAttrib3D const &v0, VertexAttrib3D const &v1, VertexAttrib3D const &v2, int32_t XMinBound,
                        int32_t XMaxBound)
{
    Platform platform = GetCurrentPlatform();
    int      width_h  = (platform.width - 1) / 2;
    int      height_h = (platform.height - 1) / 2;
    int      x0       = static_cast<int>(width_h * (v0.Position.x + 1));
    int      x1       = static_cast<int>(width_h * (v1.Position.x + 1));
    int      x2       = static_cast<int>(width_h * (v2.Position.x + 1));

    int      y0       = static_cast<int>(height_h * (1 + v0.Position.y));
    int      y1       = static_cast<int>(height_h * (1 + v1.Position.y));
    int      y2       = static_cast<int>(height_h * (1 + v2.Position.y));

    float    z0       = v0.Position.z;
    float    z1       = v1.Position.z;
    float    z2       = v2.Position.z;

    // With raster info struct now
    RasterInfo rs0(x0, y0, z0, v0.Position.w, v0.TexCoord, v0.Color);
    RasterInfo rs1(x1, y1, z1, v1.Position.w, v1.TexCoord, v1.Color);
    RasterInfo rs2(x2, y2, z2, v2.Position.w, v2.TexCoord, v2.Color);

    // Run all threads parallely from here
    // parameterized by rs0,rs1 and rs2
    //// Parallel::ParallelRasteriser(rs0,rs1,rs2);
    // auto &thread_pool = get_current_thread_pool();
    // auto &renderer    = get_current_parallel_renderer();
    //// Use the parallel renderer to render all sections in parallel using above thread_pool
    // renderer.parallel_rasterize(thread_pool, rs0, rs1, rs2);
    //  wait until all worker thread have been completed
    Parallel::Rasteriser(rs0, rs1, rs2, XMinBound, XMaxBound);
}

static void ClipSpace2D(VertexAttrib3D v0, VertexAttrib3D v1, VertexAttrib3D v2, MemAlloc<VertexAttrib3D> &allocator,
                        int32_t XMinBound, int32_t XMaxBound)
{
    v0.Position = v0.Position.PerspectiveDivide();
    v1.Position = v1.Position.PerspectiveDivide();
    v2.Position = v2.Position.PerspectiveDivide();

    std::vector<VertexAttrib3D, MemAlloc<VertexAttrib3D>> outVertices({v0, v1, v2}, allocator);
    auto                                                  inVertices = outVertices;

    std::vector<Vec2f, MemAlloc<Vec2f>>                   clipPoly(
                          {Vec2(-1.0f, 1.0f), Vec2(-1.0f, -1.0f), Vec2(1.0f, -1.0f), Vec2(1.0f, 1.0f)}, allocator);

    std::vector<Vec2f, MemAlloc<Vec2f>> clipLines(clipPoly.size(), allocator);

    for (int i = 0; i < clipPoly.size(); ++i)
        clipLines.at(i) = clipPoly.at((i + 1) % clipPoly.size()) - clipPoly.at(i);

    size_t vert = 0;
    outVertices.clear();
    outVertices = {v0, v1, v2};

    vert        = 0;
    for (const auto &line : clipLines)
    {
        if (outVertices.size() == 1)
            break;
        inVertices = outVertices;
        // If single point remained, discard it
        outVertices.clear();

        for (size_t i = 0; i < inVertices.size(); ++i)
        {
            auto const &v0 = inVertices.at(i), v1 = inVertices.at((i + 1) % inVertices.size());

            auto head = Vec2<float>::Determinant(line, Vec2f(v1.Position.x, v1.Position.y) - clipPoly[vert]) >= 0;
            auto tail = Vec2<float>::Determinant(line, Vec2f(v0.Position.x, v0.Position.y) - clipPoly[vert]) >= 0;

            if (head && tail)
                outVertices.push_back(v1);
            else if (!(head || tail))
                continue;
            else
            {
                // This edge is divided by the clip line
                auto intersection =
                    LineIntersect(clipPoly[vert], clipPoly[vert] + line, Vec2f(v0.Position.x, v0.Position.y),
                                  Vec2f(v1.Position.x, v1.Position.y));
                auto point1 = clipPoly[vert] + line * intersection.x;
                auto point2 =
                    Vec2f(v0.Position.x, v0.Position.y) +
                    (Vec2f(v1.Position.x, v1.Position.y) - Vec2f(v0.Position.x, v0.Position.y)) * intersection.y;

                // point1 and point2 are same and are the intersection obtained by clipping against the clipping Poly
                // Since they lie on the edge of the triangle, vertex attributes can be linearly interpolated across
                // those two vertices
                VertexAttrib3D inter;
                inter.Position = Vec4f(point1, 0.0f, 1.0f); // <-- Todo

                if (fabs(v1.Position.x - v0.Position.x) > fabs(v1.Position.y - v0.Position.y))
                {
                    // Use alternative interpolating scheme here
                    float t = (inter.Position.x - v0.Position.x) / (v1.Position.x - v0.Position.x);
                    // Interpolate depth, color_attributes and texture co-ordinates here
                    // Interplate z/w linearly too
                    inter.Position.w = v0.Position.w + t * (v1.Position.w - v0.Position.w);

                    // Depth linearly, color_attributes and texture co-ordinates to be interpolated non-linearly
                    inter.Position.z  = v0.Position.z + t * (v1.Position.z - v0.Position.z);

                    float one_minus_t = 1 - t;
                    t                 = t / v0.Position.w;
                    one_minus_t       = one_minus_t / v1.Position.w;

                    inter.TexCoord    = (t * v1.TexCoord + one_minus_t * v0.TexCoord);
                    inter.TexCoord    = inter.TexCoord * (1.0f / (t + one_minus_t));

                    inter.Color       = (t * v1.Color + one_minus_t * v0.Color);
                    inter.Color       = inter.Color * (1.0f / (t + one_minus_t));
                }
                else
                {
                    // Simple linear interpolation doesn't work here
                    float t          = (inter.Position.y - v0.Position.y) / (v1.Position.y - v0.Position.y);
                    inter.Position.w = v0.Position.w + t * (v1.Position.w - v0.Position.w);

                    // Depth, color_attributes and texture co-ordinates to be interpolated non-linearly
                    inter.Position.z  = v0.Position.z + t * (v1.Position.z - v0.Position.z);

                    float one_minus_t = 1 - t;
                    t                 = t / v0.Position.w;
                    one_minus_t       = one_minus_t / v1.Position.w;

                    inter.TexCoord    = (t * v1.TexCoord + one_minus_t * v0.TexCoord);
                    inter.TexCoord    = inter.TexCoord * (1.0f / (t + one_minus_t));

                    inter.Color       = (t * v1.Color + one_minus_t * v0.Color);
                    inter.Color       = inter.Color * (1.0f / (t + one_minus_t));
                }
                // Nothing done here .. Might revisit during texture mapping phase
                // TODO -> Handle cases
                // auto floatColor = Vec3f(v0.Color.x, v0.Color.y, v0.Color.z) + (inter.Position.x - v0.Position.x) /
                //                                                                  (v1.Position.x - v0.Position.x) *
                //                                                                  (v1.Color - v0.Color);
                outVertices.push_back(inter); // Pushes the intersection point
                if (head)
                    outVertices.push_back(v1);
            }
        }
        vert++;
    }

    if (outVertices.size() < 3)
        return;

    for (int vertex = 1; vertex < outVertices.size() - 1; vertex += 1)
    {
        Parallel::ScreenSpace(outVertices.at(0), outVertices.at(vertex),
                              outVertices.at((vertex + 1) % outVertices.size()), XMinBound, XMaxBound);
    }
}

static void Clip3D(VertexAttrib3D const &v0, VertexAttrib3D const &v1, VertexAttrib3D const &v2,
                   MemAlloc<Pipeline3D::VertexAttrib3D> &allocator, int32_t XMinBound, int32_t XMaxBound)
{
    // TODO :: SIMDify this
    if (v0.Position.z > v0.Position.w && v1.Position.z > v1.Position.w && v2.Position.z > v2.Position.w)
        return;
    if (v0.Position.x < -v0.Position.w && v1.Position.x < -v1.Position.w && v2.Position.x < -v2.Position.w)
        return;
    if (v0.Position.x > v0.Position.w && v1.Position.x > v1.Position.w && v2.Position.x > v2.Position.w)
        return;
    if (v0.Position.y < -v0.Position.w && v1.Position.y < -v1.Position.w && v2.Position.y < -v2.Position.w)
        return;
    if (v0.Position.y > v0.Position.w && v1.Position.y > v1.Position.w && v2.Position.y > v2.Position.w)
        return;
    if (v0.Position.z > v0.Position.w && v1.Position.z > v1.Position.w && v2.Position.z > v2.Position.w)
        return;
    if (v0.Position.z < 0 && v1.Position.z < 0 && v2.Position.z < 0)
        return;
    std::vector<VertexAttrib3D, MemAlloc<VertexAttrib3D>> inVertices({v0, v1, v2}, allocator);
    auto                                                  outVertices = inVertices;

    if (v0.Position.z < 0 || v1.Position.z < 0 || v2.Position.z < 0)
    {
        outVertices.clear();
        for (int i = 0; i < inVertices.size(); ++i)
        {
            //  __debugbreak();
            auto const &v0   = inVertices.at(i);
            auto const &v1   = inVertices.at((i + 1) % inVertices.size());
            bool        head = v1.Position.z >= 0.0f;
            // std::numeric_limits<float>::epsilon();
            bool tail = v0.Position.z >= 0.0f;
            // std::numeric_limits<float>::epsilon();
            if (head && tail)
                outVertices.push_back(v1); // <-- If both are inside the clipping near plane, add the head
            else if (!(head || tail))
                continue;
            else
            {
                // <-- If any part is inside or outside, interpolate along the z axis, all parameters
                //__debugbreak();
                float          t  = (-v0.Position.z) / (v1.Position.z - v0.Position.z);
                auto           pn = v0.Position + t * (v1.Position - v0.Position);
                VertexAttrib3D vn;
                vn.Position = pn;

                vn.TexCoord = v0.TexCoord + t * (v1.TexCoord - v0.TexCoord);
                vn.Color    = v0.Color + t * (v1.Color - v0.Color);
                outVertices.push_back(vn);
                if (head)
                    outVertices.push_back(v1);
            }
        }
    }
    for (int vertex = 1; vertex < outVertices.size() - 1; vertex += 1)
    {
        // ScreenSpace(outVertices.at(0), outVertices.at(vertex), outVertices.at((vertex + 1) %
        // outVertices.size()));
        Parallel::ClipSpace2D(outVertices.at(0), outVertices.at(vertex),
                              outVertices.at((vertex + 1) % outVertices.size()), allocator, XMinBound, XMaxBound);
    }
}

static void ParallelRenderableDraw(RenderList &renderables, MemAlloc<Pipeline3D::VertexAttrib3D> &allocator,
                                   int32_t XMinBound, int32_t XMaxBound)
{

    VertexAttrib3D v0, v1, v2;
    // Run the whole pipeline simultaneously on multiple threads
    auto device = GetRasteriserDevice();
    for (auto const &renderable : renderables.Renderables)
    {
        device->Context.ActiveMergeMode = renderable.merge_mode;
        if (renderable.merge_mode == RenderDevice::MergeMode::TEXTURE_MODE)
            SetActiveTexture(renderable.textureID);
        assert(renderable.indices.size() % 3 == 0);
        for (std::size_t i = 0; i < renderable.indices.size(); i += 3)
        {
            allocator.resource->reset();
            v0          = renderable.vertices[renderable.indices[i]];
            v1          = renderable.vertices[renderable.indices[i + 1]];
            v2          = renderable.vertices[renderable.indices[i + 2]];

            v0.Position = renderable.scene_transform * v0.Position;
            v1.Position = renderable.scene_transform * v1.Position;
            v2.Position = renderable.scene_transform * v2.Position;

            Parallel::Clip3D(v0, v1, v2, allocator, XMinBound, XMaxBound);
        }
    }
}

// Try packing all these arguments
void ParallelTypeErasedDraw(void *arg)
{
    // Retrieve back the type erased information
    auto drawArgs = static_cast<Parallel::ParallelRenderer::ParallelThreadArgStruct *>(arg);
    /*ParallelDraw(*drawArgs->vertex_vector, *drawArgs->index_vector, *drawArgs->matrix, *drawArgs->allocator,
                 drawArgs->XMinBound, drawArgs->XMaxBound);*/
    ParallelRenderableDraw(*drawArgs->render_list, *drawArgs->allocator, drawArgs->XMinBound, drawArgs->XMaxBound);
}

// Parallel Renderer class definition
ParallelRenderer::ParallelRenderer(const int width, const int height)
{
    // Change of idea
    // We will partition the screen into 4 vertical segments where each thread will work on each thread
    // It will make checking for y co-ordinates redundant
    boundary[0]   = 0;
    int32_t first = width / no_of_partitions;

    for (auto i = 1; i < no_of_partitions; ++i)
        boundary[i] = i * first;

    boundary[no_of_partitions] = width;
}

void ParallelRenderer::AlternativeParallelRenderablePipeline(
    Alternative::ThreadPool &thread_pool, RenderList &renderables,
    std::vector<MemAlloc<Pipeline3D::VertexAttrib3D>> &allocator)
{
    std::latch waiter(no_of_partitions);
    // Allocate objects on the stack
    ParallelThreadArgStruct args[no_of_partitions];

    for (auto i : std::ranges::iota_view(1u, no_of_partitions + 1))
    {
        args[i - 1] = ParallelThreadArgStruct(&renderables, &allocator[i], boundary[i - 1], boundary[i]);
    }

    // operate on passive ptr here
    auto count = 0u;
    for (auto &task : *thread_pool.get_passive_ptr())
    {
        /*task = Alternative::ThreadPool::AlternativeTaskDesc{
            false, Alternative::ThreadPool::ThreadPoolFunc(Parallel::ParallelTypeErasedDraw, &args[count++])};*/
        task.completed = false;
        task.task      = Alternative::ThreadPool::ThreadPoolFunc(Parallel::ParallelTypeErasedDraw, &args[count++]);
    }

    thread_pool.started(&waiter);
    thread_pool.wait_till_finished();
}

} // namespace Parallel