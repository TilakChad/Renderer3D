#include "../../include/rasteriser.h"
#include "../../image/PNGLoader.h"
#include "../../maths/vec.hpp"

#include "../../utils/thread_pool.h"
#include "../../utils/parallel_render.h"
#include <algorithm>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

static thread_local RenderDevice                Device;

extern Parallel::ParallelRenderer &get_current_parallel_renderer();
extern ThreadPool                 &get_current_thread_pool();

void Rasteriser(int x1, int y1, int x2, int y2, int x3, int y3, Vec4f attribA, Vec4f attribB, Vec4f attribC, Vec2f texA,
                Vec2f texB, Vec2f texC)
{
    // Its the triangle rasteriser
    // It can optionally cull front face, back face or null
    Platform platform = GetCurrentPlatform();

    int      minX     = std::min({x1, x2, x3});
    int      maxX     = std::max({x1, x2, x3});

    int      minY     = std::min({y1, y2, y3});
    int      maxY     = std::max({y1, y2, y3});

    // Assume vectors are in clockwise ordering
    Vec2     v0   = Vec2(x2, y2) - Vec2(x1, y1);
    Vec2     v1   = Vec2(x3, y3) - Vec2(x2, y2);
    Vec2     v2   = Vec2(x1, y1) - Vec2(x3, y3);

    Vec2     vec1 = Vec2(x1, y1);
    Vec2     vec2 = Vec2(x2, y2);
    Vec2     vec3 = Vec2(x3, y3);

    uint8_t *mem;
    // potential for paralellization
    int32_t area = Vec2<int>::Determinant(v1, v0);

    if (area > 0 && Device.Context.ActiveRasteriserMode == RenderDevice::RasteriserMode::FRONT_FACE_CULL)
        return;
    if (area < 0 && Device.Context.ActiveRasteriserMode == RenderDevice::RasteriserMode::BACK_FACE_CULL)
        return;

    float    l1, l2, l3; // Barycentric coordinates

    Vec2     point = Vec2(minX, minY);
    auto     a1_   = Vec2<int32_t>::Determinant(point - vec1, v0);
    auto     a2_   = Vec2<int32_t>::Determinant(point - vec2, v1);
    auto     a3_   = Vec2<int32_t>::Determinant(point - vec3, v2);
    int32_t  a1, a2, a3;
    uint32_t stride = platform.colorBuffer.width * platform.colorBuffer.noChannels;

    // Code duplication better than per pixel check
    if (Device.Context.ActiveMergeMode == RenderDevice::MergeMode::COLOR_MODE)
    {
        //// My optimized rasterization
        //
        for (int h = minY; h <= maxY; ++h)
        {
            int32_t offset = (platform.colorBuffer.height - 1 - h) * stride;
            mem            = platform.colorBuffer.buffer + offset;
            mem            = mem + minX * platform.colorBuffer.noChannels;

            a1             = a1_;
            a2             = a2_;
            a3             = a3_;

            // This is a very tight loop and need to be optimized heavily even for a suitable framerate
            for (int w = minX; w <= maxX; ++w)
            {
                if (((a1 | a2 | a3) >= 0) || ((a1 & a2 & a3) < 0)) // --> Turns on rasterization of
                                                                   //  anti clockwise triangles
                {
                    l1 = static_cast<float>(a2) / area;
                    l2 = static_cast<float>(a3) / area;
                    l3 = static_cast<float>(a1) / area;

                    // Need to vectorize this things after I learn some SIMD instructions first .. Let's go with it for
                    // now
                    Vec4f src_color = l1 * attribA + l2 * attribB + l3 * attribC;

                    mem[0]          = static_cast<uint8_t>(src_color.z * 255);
                    mem[1]          = static_cast<uint8_t>(src_color.y * 255);
                    mem[2]          = static_cast<uint8_t>(src_color.x * 255);
                    mem[3]          = static_cast<uint8_t>(src_color.w * 255);
                }
                a1  = a1 + v0.y;
                a2  = a2 + v1.y;
                a3  = a3 + v2.y;
                mem = mem + 4;
            }
            a1_ = a1_ - v0.x;
            a2_ = a2_ - v1.x;
            a3_ = a3_ - v2.x;
        }
    }
    // Texture texture;
    else if (Device.Context.ActiveMergeMode == RenderDevice::MergeMode::BLEND_MODE)
    {
        for (int h = minY; h <= maxY; ++h)
        {
            int32_t offset =
                (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width * platform.colorBuffer.noChannels;
            mem = platform.colorBuffer.buffer + offset;
            mem = mem + minX * platform.colorBuffer.noChannels;
            a1  = a1_;
            a2  = a2_;
            a3  = a3_;

            // This is a very tight loop and need to be optimized heavily even for a suitable framerate
            for (int w = minX; w <= maxX; ++w)
            {
                if ((a1 | a2 | a3) >= 0 || (a1 & a2 & a3) < 0) // --> Turns on rasterization of
                                                               //  anti clockwise triangles
                {

                    // SRC_ALPHA and ONE_MINUS_SRC_ALPHA blending

                    l1 = static_cast<float>(a2) / area;
                    l2 = static_cast<float>(a3) / area;
                    l3 = static_cast<float>(a1) / area;

                    // MAYDO : Switch to pre-multiplied alpha
                    Vec4f src   = l1 * attribA + l2 * attribB + l3 * attribC;
                    Vec4f dest  = Vec4f(mem[2] / 255.0f, mem[1] / 255.0f, mem[0] / 255.0f, mem[3] / 255.0f);

                    auto  alpha = src.w + dest.w * (1 - src.w);

                    auto  color = src * src.w + dest.w * (1 - src.w) * dest;
                    color       = color * (1.0f / alpha);

                    mem[0]      = static_cast<uint8_t>(color.z * 255);
                    mem[1]      = static_cast<uint8_t>(color.y * 255);
                    mem[2]      = static_cast<uint8_t>(color.x * 255);
                    mem[3]      = static_cast<uint8_t>(alpha * 255);
                }
                a1  = a1 + v0.y;
                a2  = a2 + v1.y;
                a3  = a3 + v2.y;
                mem = mem + 4;
            }
            a1_ = a1_ - v0.x;
            a2_ = a2_ - v1.x;
            a3_ = a3_ - v2.x;
        }
    }
    else if (Device.Context.ActiveMergeMode == RenderDevice::MergeMode::TEXTURE_MODE)
    {
        auto texture = Device.Context.GetActiveTexture();
        for (int h = minY; h <= maxY; ++h)
        {
            int32_t offset =
                (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width * platform.colorBuffer.noChannels;
            mem = platform.colorBuffer.buffer + offset;
            mem = mem + minX * platform.colorBuffer.noChannels;

            a1  = a1_;
            a2  = a2_;
            a3  = a3_;

            // This is a very tight loop and need to be optimized heavily even for a suitable framerate
            for (int w = minX; w <= maxX; ++w)
            {
                if ((a1 | a2 | a3) >= 0 ||
                    (a1 & a2 & a3) < 0) // --> Turns on rasterization of //  anti clockwise triangles
                {

                    l1       = static_cast<float>(a2) / area;
                    l2       = static_cast<float>(a3) / area;
                    l3       = static_cast<float>(a1) / area;

                    auto uv  = l1 * texA + l2 * texB + l3 * texC;
                    auto rgb = texture.Sample(Vec2(uv), Texture::Interpolation::NEAREST);
                    mem[0]   = rgb.z;
                    mem[1]   = rgb.y;
                    mem[2]   = rgb.x;
                    mem[3]   = 0x00;
                }
                a1  = a1 + v0.y;
                a2  = a2 + v1.y;
                a3  = a3 + v2.y;
                mem = mem + 4;
            }
            a1_ = a1_ - v0.x;
            a2_ = a2_ - v1.x;
            a3_ = a3_ - v2.x;
        }
    }
    else if (Device.Context.ActiveMergeMode == RenderDevice::MergeMode::TEXTURE_BLENDING_MODE)
    {
        auto texture = Device.Context.GetActiveTexture();
        for (int h = minY; h <= maxY; ++h)
        {
            int32_t offset =
                (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width * platform.colorBuffer.noChannels;
            mem = platform.colorBuffer.buffer + offset;
            mem = mem + minX * platform.colorBuffer.noChannels;

            a1  = a1_;
            a2  = a2_;
            a3  = a3_;

            // This is a very tight loop and need to be optimized heavily even for a suitable framerate
            for (int w = minX; w <= maxX; ++w)
            {
                if ((a1 >= 0 && a2 >= 0 && a3 >= 0) || (a1 < 0 && a2 < 0 && a3 < 0)) // --> Turns on rasterization of
                                                                                     //  anti clockwise triangles
                {

                    l1          = static_cast<float>(a2) / area;
                    l2          = static_cast<float>(a3) / area;
                    l3          = static_cast<float>(a1) / area;

                    auto  uv    = l1 * texA + l2 * texB + l3 * texC;
                    auto  rgb   = texture.Sample(Vec2(uv), Texture::Interpolation::NEAREST);

                    Vec4f src   = l1 * attribA + l2 * attribB + l3 * attribC;
                    src         = Vec4f(rgb * 1.0f / 255.0f, src.w);
                    Vec4f dest  = Vec4f(mem[2] / 255.0f, mem[1] / 255.0f, mem[0] / 255.0f, mem[3] / 255.0f);

                    auto  alpha = src.w + dest.w * (1 - src.w);

                    auto  color = src * src.w + dest.w * (1 - src.w) * dest;
                    color       = color * (1.0f / alpha);

                    mem[0]      = color.z * 255;
                    mem[1]      = color.y * 255;
                    mem[2]      = color.x * 255;
                    mem[3]      = alpha * 255;
                }
                a1  = a1 + v0.y;
                a2  = a2 + v1.y;
                a3  = a3 + v2.y;
                mem = mem + 4;
            }

            a1_ = a1_ - v0.x;
            a2_ = a2_ - v1.x;
            a3_ = a3_ - v2.x;
        }
    }
}

void ScreenSpace(Pipeline2D::VertexAttrib2D v0, Pipeline2D::VertexAttrib2D v1, Pipeline2D::VertexAttrib2D v2)
{
    // ClipSpace({x1, y1}, {x2, y2}, {x3, y3});
    Platform platform = GetCurrentPlatform();
    int      scrX0    = static_cast<int>((platform.width - 1) / 2 * (v0.Position.x + 1));
    int      scrY0    = static_cast<int>((platform.height - 1) / 2 * (1 + v0.Position.y));

    int      scrX1    = static_cast<int>((platform.width - 1) / 2 * (v1.Position.x + 1));
    int      scrY1    = static_cast<int>((platform.height - 1) / 2 * (1 + v1.Position.y));

    int      scrX2    = static_cast<int>((platform.width - 1) / 2 * (v2.Position.x + 1));
    int      scrY2    = static_cast<int>((platform.height - 1) / 2 * (1 + v2.Position.y));

    // Rasteriser(scrX0, scrY0, scrX1, scrY1, scrX2, scrY2, v0.Color, v1.Color, v2.Color);
    Rasteriser(scrX0, scrY0, scrX1, scrY1, scrX2, scrY2, v0.Color, v1.Color, v2.Color, v0.TexCoord, v1.TexCoord,
               v2.TexCoord);
}

// Polygon Clipping using Sutherland-Hodgeman algorithm in NDC space before screen space mapping

void ClipSpace(Pipeline2D::VertexAttrib2D v0, Pipeline2D::VertexAttrib2D v1, Pipeline2D::VertexAttrib2D v2)
{
    // Triangle defined by vertices v0, v1, and v2
    /*
    (-1,1)                 (1,1)
    ______________________
    |                     |
    |                     |
    |                     |
    |                     |
    |                     |
    |                     |
    |_____________________|
 (-1,-1)                (1,-1)
    */
    std::vector<Pipeline2D::VertexAttrib2D> outVertices{v0, v1, v2};
    outVertices.reserve(5);

    std::vector<Pipeline2D::VertexAttrib2D> inVertices{};
    inVertices.reserve(5);

    auto const clipPoly =
        std::vector<Vec2<float32>>{Vec2(-1.0f, 1.0f), Vec2(-1.0f, -1.0f), Vec2(1.0f, -1.0f), Vec2(1.0f, 1.0f)};

    std::vector<Vec2<float32>> clipLines(clipPoly.size());

    for (int i = 0; i < clipPoly.size(); ++i)
        clipLines.at(i) = clipPoly.at((i + 1) % clipPoly.size()) - clipPoly.at(i);

    size_t vert = 0;

    // Standard Sutherland-Hodgeman Algorithm
    // for (auto const &line : clipLines)
    //{
    //    inVertices = outVertices;
    //    outVertices.clear();

    //    // First choose vertices from the polygon going to be clipped
    //    for (size_t i = 0; i < inVertices.size(); ++i)
    //    {
    //        auto v0 = inVertices.at(i), v1 = inVertices.at((i + 1) % inVertices.size());

    //        if (Vec2<float32>::Determinant(line, v1 - v0) == 0) // implies no intersection
    //            continue;

    //        auto intersection = LineIntersect(clipPoly[vert], clipPoly[vert] + line, v0, v1);
    //        auto point1       = clipPoly[vert] + line * intersection.x;
    //        auto point2       = v0 + (v1 - v0) * intersection.y;

    //        if (Vec2<float32>::Determinant(line, v1 - clipPoly[vert]) >= 0) // if current point is inside the clip
    //        // line
    //        {
    //            if (Vec2<float32>::Determinant(line, v0 - clipPoly[vert]) < 0)
    //            {
    //                outVertices.push_back(point1);
    //            }
    //            outVertices.push_back(v1);
    //        }
    //        else if (Vec2<float32>::Determinant(line, v0 - clipPoly[vert]) >= 0)
    //        {
    //            outVertices.push_back(point1);
    //        }
    //    }
    //    vert++;
    //}
    // for (auto const &i : outVertices)
    //    std::cout << i;

    outVertices.clear();
    outVertices = {v0, v1, v2};

    // My own variation which apparently is faster
    vert = 0;
    for (const auto &line : clipLines)
    {
        if (outVertices.size() == 1)
            break;
        inVertices = outVertices;
        // If single point remained, discard it
        outVertices.clear();

        for (size_t i = 0; i < inVertices.size(); ++i)
        {
            auto const& v0 = inVertices.at(i), v1 = inVertices.at((i + 1) % inVertices.size());

            auto head = Vec2<float>::Determinant(line, v1.Position - clipPoly[vert]) >= 0;
            auto tail = Vec2<float>::Determinant(line, v0.Position - clipPoly[vert]) >= 0;

            if (head && tail)
                outVertices.push_back(v1);
            else if (!(head || tail))
                continue;
            else
            {
                // This edge is divided by the clip line
                auto intersection = LineIntersect(clipPoly[vert], clipPoly[vert] + line, v0.Position, v1.Position);
                auto point1       = clipPoly[vert] + line * intersection.x;
                auto point2       = v0.Position + (v1.Position - v0.Position) * intersection.y;

                // point1 and point2 are same and are the intersection obtained by clipping against the clipping Poly
                // Since they lie on the edge of the triangle, vertex attributes can be linearly interpolated across
                // those two vertices
                Pipeline2D::VertexAttrib2D inter;
                inter.Position = point1;

                if (fabs(v1.Position.x - v0.Position.x) > fabs(v1.Position.y - v0.Position.y))
                {
                    inter.TexCoord = Vec2f(v0.TexCoord.x, v0.TexCoord.y) + (inter.Position.x - v0.Position.x) /
                                                                               (v1.Position.x - v0.Position.x) *
                                                                               (v1.TexCoord - v0.TexCoord);
                    inter.Color = v0.Color + (inter.Position.x - v0.Position.x) / (v1.Position.x - v0.Position.x) *
                                                 (v1.Color - v0.Color);
                }
                else
                {
                    inter.TexCoord = Vec2f(v0.TexCoord.x, v0.TexCoord.y) + (inter.Position.y - v0.Position.y) /
                                                                               (v1.Position.y - v0.Position.y) *
                                                                               (v1.TexCoord - v0.TexCoord);
                    inter.Color = v0.Color + (inter.Position.y - v0.Position.y) / (v1.Position.y - v0.Position.y) *
                                                 (v1.Color - v0.Color);
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

    /*assert(outVertices.size() > 2);*/ // <-- Don't exit but return (i.e Don't rasterize)
    if (outVertices.size() < 3)
        return;
    if (outVertices.size() >= 5)
        volatile int a = 5;
    auto i = 0;
    // for (int i = 0; i < outVertices.size() - 1; i += 2)
    //{
    //    ScreenSpace(outVertices.at(i), outVertices.at(i + 1), outVertices.at((i + 2) % outVertices.size()));
    //}
    // Lets try drawing a fan instead of consecutive closed triangles

    for (int vertex = 1; vertex < outVertices.size() - 1; vertex += 1)
    {
        ScreenSpace(outVertices.at(0), outVertices.at(vertex), outVertices.at((vertex + 1) % outVertices.size()));
    }
}

void RenderDevice::Draw(Pipeline2D::VertexAttrib2D const &v0, Pipeline2D::VertexAttrib2D const &v1,
                        Pipeline2D::VertexAttrib2D const &v2)
{
    // Pass through transform matrix first
    auto pos0    = Device.Context.SceneMatrix * Vec4(v0.Position, 0.0f, 1.0f);
    auto pos1    = Device.Context.SceneMatrix * Vec4(v1.Position, 0.0f, 1.0f);
    auto pos2    = Device.Context.SceneMatrix * Vec4(v2.Position, 0.0f, 1.0f);

    auto nv0     = v0;
    nv0.Position = Vec2f(pos0.x, pos0.y);
    auto nv1     = v1;
    nv1.Position = Vec2f(pos1.x, pos1.y);
    auto nv2     = v2;
    nv2.Position = Vec2f(pos2.x, pos2.y);

    ClipSpace(nv0, nv1, nv2);
}

RenderDevice *GetRasteriserDevice()
{
    return &Device;
}

void RenderDevice::CTX::SetMergeMode(MergeMode mode)
{
    ActiveMergeMode = mode;
}

void BresenhamLineRasteriser(Pipeline2D::VertexAttrib2D const &v0, Pipeline2D::VertexAttrib2D const &v1)
{
    // Line clipping first
    auto const clipPoly =
        std::vector<Vec2<float32>>{Vec2(-1.0f, 1.0f), Vec2(-1.0f, -1.0f), Vec2(1.0f, -1.0f), Vec2(1.0f, 1.0f)};

    std::vector<Vec2<float32>> clipLines(clipPoly.size());

    for (int i = 0; i < clipPoly.size(); ++i)
        clipLines.at(i) = clipPoly.at((i + 1) % clipPoly.size()) - clipPoly.at(i);

    size_t                                  vert = 0;

    std::vector<Pipeline2D::VertexAttrib2D> outVertices;
    std::vector<Pipeline2D::VertexAttrib2D> inVertices;

    outVertices.clear();
    outVertices = {v0, v1};

    vert        = 0;
    for (const auto &line : clipLines)
    {
        if (outVertices.size() == 1)
            break;
        inVertices = outVertices;
        outVertices.clear();

        for (size_t i = 0; i < inVertices.size(); ++i)
        {
            auto const& v0 = inVertices.at(i), v1 = inVertices.at((i + 1) % inVertices.size());

            auto head = Vec2<float>::Determinant(line, v1.Position - clipPoly[vert]) >= 0;
            auto tail = Vec2<float>::Determinant(line, v0.Position - clipPoly[vert]) >= 0;

            if (head && tail)
                outVertices.push_back(v1);
            else if (!(head || tail))
                continue;
            else
            {
                // This edge is divided by the clip line
                auto intersection = LineIntersect(clipPoly[vert], clipPoly[vert] + line, v0.Position, v1.Position);
                auto point1       = clipPoly[vert] + line * intersection.x;

                Pipeline2D::VertexAttrib2D inter;
                inter.Position = point1;

                if (fabs(v1.Position.x - v0.Position.x) > fabs(v1.Position.y - v0.Position.y))

                    inter.Color = v0.Color + (inter.Position.x - v0.Position.x) / (v1.Position.x - v0.Position.x) *
                                                 (v1.Color - v0.Color);
                else
                    inter.Color = v0.Color + (inter.Position.y - v0.Position.y) / (v1.Position.y - v0.Position.y) *
                                                 (v1.Color - v0.Color);

                outVertices.push_back(inter);
                if (head)
                    outVertices.push_back(v1);
            }
        }
        vert++;
    }
    if (outVertices.empty())
        return;

    auto const&     v2       = outVertices.back();
    auto const&    v3       = outVertices.front();

    Platform platform = GetCurrentPlatform();
    int      x1       = static_cast<int>((platform.width - 1) / 2 * (v2.Position.x + 1));
    int      y1       = static_cast<int>((platform.height - 1) / 2 * (1 + v2.Position.y));

    int      x2       = static_cast<int>((platform.width - 1) / 2 * (v3.Position.x + 1));
    int      y2       = static_cast<int>((platform.height - 1) / 2 * (1 + v3.Position.y));

    // Use Bresenham's line drawing algo to draw
    // Find the normal vector to the line
    Vec2f vec1(x1, y1);
    Vec2f vec2(x2, y2);
    auto  unit          = (vec2 - vec1);
    unit                = unit * (1.0f / unit.Norm());
    Vec2f normal        = Vec2f(unit.y, -unit.x);

    auto  thickness     = 5.0f; // Lets leave it here .. Not working further on it

    auto  BresenhamLine = [=](int x1, int y1, int x2, int y2) {
        int dx   = abs(x2 - x1);
        int dy   = abs(y2 - y1);
        int xinc = 1, yinc = 1;

        if (x2 < x1)
            xinc = -1;
        if (y2 < y1)
            yinc = -1;

        int  x = x1, y = y1;

        auto plotPixel = [=](uint32_t x, uint32_t y, Vec4f color) {
            uint8_t *pixel =
                platform.colorBuffer.buffer +
                ((platform.colorBuffer.height - 1 - y) * platform.width + x) * platform.colorBuffer.noChannels;
            pixel[0] = color.z * 255;
            pixel[1] = color.y * 255;
            pixel[2] = color.x * 255;
            pixel[3] = color.w * 255;
        };

        plotPixel(x, y, Vec4(1.0f, 0.0f, 0.0f, 1.0f));
        if (dy < dx)
        {
            // case with slope < 1
            int p = 2 * dy - dx;
            int i = 0;
            while (i++ < dx)
            {
                if (p < 0)
                {
                    x = x + xinc;
                    p += 2 * dy;
                }
                else
                {
                    x = x + xinc;
                    y = y + yinc;
                    p += 2 * dy - 2 * dx;
                }
                plotPixel(x, y, Vec4(1.0f, 0.0f, 0.0f, 1.0f));
            }
        }
        else
        {
            int p = 2 * dx - dy;
            int i = 0;
            while (i++ < dy)
            {
                if (p < 0)
                {
                    y = y + yinc;
                    p += 2 * dx;
                }
                else
                {
                    x = x + xinc;
                    y = y + yinc;
                    p += 2 * dx - 2 * dy;
                }
                plotPixel(x, y, Vec4(1.0f, 0.0f, 0.0f, 1.0f));
            }
        }
    };
    for (int i = -thickness / 2; i < thickness / 2; ++i)
    {
        BresenhamLine(x1 + i * normal.x, y1 + i * normal.y, x2 + i * normal.x, y2 + i * normal.y);
    }
}

void RenderDevice::Draw(Pipeline2D::VertexAttrib2D const &v0, Pipeline2D::VertexAttrib2D const &v1)
{
    BresenhamLineRasteriser(v0, v1);
}

namespace Pipeline3D
{
void ORasteriser(int32_t x0, int32_t y0, float z0, float inv_w0, int32_t x1, int32_t y1, float z1, float inv_w1,
                 int32_t x2, int32_t y2, float z2, float inv_w2, Vec4f colorA, Vec4f colorB, Vec4f colorC, Vec2f texA,
                 Vec2f texB, Vec2f texC)
{
    Platform platform = GetCurrentPlatform();

    int      minX     = std::min({x0, x1, x2});
    int      maxX     = std::max({x0, x1, x2});

    int      minY     = std::min({y0, y1, y2});
    int      maxY     = std::max({y0, y1, y2});

    // Assume vectors are in clockwise ordering
    Vec2  v0   = Vec2(x1, y1) - Vec2(x0, y0);
    Vec2  v1   = Vec2(x2, y2) - Vec2(x1, y1);
    Vec2  v2   = Vec2(x0, y0) - Vec2(x2, y2);

    Vec2  vec0 = Vec2(x0, y0);
    Vec2  vec1 = Vec2(x1, y1);
    Vec2  vec2 = Vec2(x2, y2);

    float area = Vec2<int>::Determinant(v1, v0);

    // Perspective depth interpolation, perspective correct texture interpolation ....
    Vec2 point = Vec2(minX, minY);
    // Start collecting points from here

    float a1_ = Vec2<int32_t>::Determinant(point - vec0, v0);
    float a2_ = Vec2<int32_t>::Determinant(point - vec1, v1);
    float a3_ = Vec2<int32_t>::Determinant(point - vec2, v2);

    // Read to process 4 pixels at a time
    SIMD::Vec4ss a1_vec  = SIMD::Vec4ss(a1_, a1_ + v0.y, a1_ + 2 * v0.y, a1_ + 3 * v0.y);
    SIMD::Vec4ss a2_vec  = SIMD::Vec4ss(a2_, a2_ + v1.y, a2_ + 2 * v1.y, a2_ + 3 * v1.y);
    SIMD::Vec4ss a3_vec  = SIMD::Vec4ss(a3_, a3_ + v2.y, a3_ + 2 * v2.y, a3_ + 3 * v2.y);

    SIMD::Vec4ss inc_a1  = SIMD::Vec4ss(4 * v0.y);
    SIMD::Vec4ss inc_a2  = SIMD::Vec4ss(4 * v1.y);
    SIMD::Vec4ss inc_a3  = SIMD::Vec4ss(4 * v2.y);

    SIMD::Vec4ss inc_a1_ = SIMD::Vec4ss(-v0.x);
    SIMD::Vec4ss inc_a2_ = SIMD::Vec4ss(-v1.x);
    SIMD::Vec4ss inc_a3_ = SIMD::Vec4ss(-v2.x);

    SIMD::Vec4ss a1, a2, a3;
    SIMD::Vec4ss zvec(z0, z1, z2, 0.0f);
    zvec = zvec * (1.0 / area);
    // Now processing downwards

    constexpr int hStepSize = 4;
    if (Device.Context.ActiveMergeMode == RenderDevice::MergeMode::TEXTURE_MODE) // depth mode say
    {
        // TODO :: Tile based rendering
        for (size_t h = minY; h <= maxY; ++h)
        {
            // Start of edge function calculated
            a1 = a1_vec;
            a2 = a2_vec;
            a3 = a3_vec;

            // This is a very tight loop and need to be optimized heavily even for a suitable framerate
            for (size_t w = minX; w <= maxX; w += hStepSize)
            {
                // At each step calculate the positivity or negativity now
                // Check how many a1,a2 and a3 are positive using SIMD masks
                // Don't know how to yet
                // auto mask = std::max(SIMD::Vec4ss::generate_mask(a1, a2, a3),
                // SIMD::Vec4ss::generate_neg_mask(a1,a2,a3));
                // depending on this mask plot pixel
                auto mask = SIMD::Vec4ss::generate_mask(a1, a2, a3);
                if (mask > 0)
                {

                    size_t offset = (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width *
                                    platform.colorBuffer.noChannels;
                    uint8_t *off   = platform.colorBuffer.buffer + offset + w * platform.colorBuffer.noChannels;
                    uint8_t *mem   = off;
                    auto     depth = &platform.zBuffer.buffer[h * platform.zBuffer.width + w];

                    // Do barycentric calculations here and incrementally increase
                    // Didn't find a better way, so offloading every array elements
                    // What we need is the transpose of these matrices .. not using float load ... penality on simd
                    // operations
                    // float simd[3][4];
                    //_mm_store_ps(simd[0], a1.vec);
                    //_mm_store_ps(simd[1], a2.vec);
                    //_mm_store_ps(simd[2], a3.vec);
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

                    // This shuffle should be avoidable using above operations on different order
                    // Next time
                    lvec1 = Vec4ss(_mm_shuffle_ps(lvec1.vec, lvec1.vec, _MM_SHUFFLE(2, 1, 3, 0)));
                    lvec2 = Vec4ss(_mm_shuffle_ps(lvec2.vec, lvec2.vec, _MM_SHUFFLE(2, 1, 3, 0)));
                    lvec3 = Vec4ss(_mm_shuffle_ps(lvec3.vec, lvec3.vec, _MM_SHUFFLE(2, 1, 3, 0)));
                    lvec4 = Vec4ss(_mm_shuffle_ps(lvec4.vec, lvec4.vec, _MM_SHUFFLE(2, 1, 3, 0)));
                    //
                    // if (mask & 0x08)
                    //{
                    //    // plot first pixel
                    //    // calculate z

                    //    float z = SIMD::Vec4ss(simd[1][3], simd[2][3], simd[0][3], 0.0f).dot(zvec);
                    //    if (z < depth[0])
                    //    {
                    //        depth[0] = z;
                    //        mem[0]   = z * 255;
                    //        mem[1]   = z * 255;
                    //        mem[2]   = z * 255;
                    //        mem[3]   = 0x00;
                    //    }
                    //}
                    // if (mask & 0x04)
                    //{
                    //    // plot second pixel
                    //    mem     = off + 4;

                    //    float z = SIMD::Vec4ss(simd[1][2], simd[2][2], simd[0][2], 0.0f).dot(zvec);
                    //    if (z < depth[1])
                    //    {
                    //        depth[1] = z;

                    //        mem[0]   = z * 255;
                    //        mem[1]   = z * 255;
                    //        mem[2]   = z * 255;
                    //        mem[3]   = 0x00;
                    //    }
                    //}
                    // if (mask & 0x02)
                    //{
                    //    // plot third pixel
                    //    mem     = off + 8;

                    //    float z = SIMD::Vec4ss(simd[1][1], simd[2][1], simd[0][1], 0.0f).dot(zvec);
                    //    if (z < depth[2])
                    //    {
                    //        depth[2] = z;

                    //        mem[0]   = z * 255;
                    //        mem[1]   = z * 255;
                    //        mem[2]   = z * 255;
                    //        mem[3]   = 0x00;
                    //    }
                    //}
                    // if (mask & 0x01)
                    //{
                    //    // plot fourth pixel
                    //    mem     = off + 12;

                    //    float z = SIMD::Vec4ss(simd[1][0], simd[2][0], simd[0][0], 0.0f).dot(zvec);
                    //    if (z < depth[3])
                    //    {
                    //        depth[3] = z;
                    //        mem[0]   = z * 255;
                    //        mem[1]   = z * 255;
                    //        mem[2]   = z * 255;
                    //        mem[3]   = 0x00;
                    //    }
                    //}
                    if (mask & 0x08)
                    {
                        // plot first pixel
                        // calculate z

                        float z = lvec4.dot(zvec);
                        if (z < depth[0])
                        {
                            depth[0] = z;
                            mem[0]   = z * 255;
                            mem[1]   = z * 255;
                            mem[2]   = z * 255;
                            mem[3]   = 0x00;
                        }
                    }
                    if (mask & 0x04)
                    {
                        // plot second pixel
                        mem     = off + 4;

                        float z = lvec3.dot(zvec);
                        if (z < depth[1])
                        {
                            depth[1] = z;

                            mem[0]   = z * 255;
                            mem[1]   = z * 255;
                            mem[2]   = z * 255;
                            mem[3]   = 0x00;
                        }
                    }
                    if (mask & 0x02)
                    {
                        // plot third pixel
                        mem     = off + 8;

                        float z = lvec2.dot(zvec);
                        if (z < depth[2])
                        {
                            depth[2] = z;

                            mem[0]   = z * 255;
                            mem[1]   = z * 255;
                            mem[2]   = z * 255;
                            mem[3]   = 0x00;
                        }
                    }
                    if (mask & 0x01)
                    {
                        // plot fourth pixel
                        mem     = off + 12;

                        float z = lvec1.dot(zvec);
                        if (z < depth[3])
                        {
                            depth[3] = z;
                            mem[0]   = z * 255;
                            mem[1]   = z * 255;
                            mem[2]   = z * 255;
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

void Rasteriser(int32_t x0, int32_t y0, float z0, float inv_w0, int32_t x1, int32_t y1, float z1, float inv_w1,
                int32_t x2, int32_t y2, float z2, float inv_w2, Vec4f colorA, Vec4f colorB, Vec4f colorC, Vec2f texA,
                Vec2f texB, Vec2f texC)
{
    // Its the triangle rasteriser
    // It can optionally cull front face, back face or null
    Platform platform = GetCurrentPlatform();

    int      minX     = std::min({x0, x1, x2});
    int      maxX     = std::max({x0, x1, x2});

    int      minY     = std::min({y0, y1, y2});
    int      maxY     = std::max({y0, y1, y2});

    // Assume vectors are in clockwise ordering
    Vec2     v0   = Vec2(x1, y1) - Vec2(x0, y0);
    Vec2     v1   = Vec2(x2, y2) - Vec2(x1, y1);
    Vec2     v2   = Vec2(x0, y0) - Vec2(x2, y2);

    Vec2     vec0 = Vec2(x0, y0);
    Vec2     vec1 = Vec2(x1, y1);
    Vec2     vec2 = Vec2(x2, y2);

    uint8_t *mem;
    // potential for paralellization
    float area = Vec2<int>::Determinant(v1, v0);

    // Perspective depth interpolation, perspective correct texture interpolation ....
    Vec2               point = Vec2(minX, minY);
    float              a1_   = Vec2<int32_t>::Determinant(point - vec0, v0);
    float              a2_   = Vec2<int32_t>::Determinant(point - vec1, v1);
    float              a3_   = Vec2<int32_t>::Determinant(point - vec2, v2);

    SIMD::Vec4ss       a, a_(a1_, a2_, a3_, 0.0f);
    SIMD::Vec4ss       l;
    const SIMD::Vec4ss z_vec     = SIMD::Vec4ss(z0, z1, z2, 0.0f);
    const SIMD::Vec4ss inv_w     = SIMD::Vec4ss(inv_w0, inv_w1, inv_w2, 0.0f);
    const SIMD::Vec4ss inc_ry    = SIMD::Vec4ss(v0.y, v1.y, v2.y, 0.0f);
    const SIMD::Vec4ss inc_rx    = SIMD::Vec4ss(v0.x, v1.x, v2.x, 0.0f);
    uint32_t           stride    = platform.colorBuffer.width * platform.colorBuffer.noChannels;

    const SIMD::Vec4ss TexA_simd = SIMD::Vec4ss(texA.x, texA.y, 0.0f, 0.0f);
    const SIMD::Vec4ss TexB_simd = SIMD::Vec4ss(texB.x, texB.y, 0.0f, 0.0f);
    const SIMD::Vec4ss TexC_simd = SIMD::Vec4ss(texC.x, texC.y, 0.0f, 0.0f);

    // Parallelization using SIMD vectorization
    float a1, a2, a3;
    // Code duplication better than per pixel check
    // if (Device.Context.ActiveMergeMode == RenderDevice::MergeMode::TEXTURE_MODE)
    {
        // My optimized rasterization
        // auto texture = Device.Context.GetActiveTexture();
        for (size_t h = minY; h <= maxY; ++h)
        { /*
             size_t offset =
                 (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width *
     platform.colorBuffer.noChannels;
             mem = platform.colorBuffer.buffer + offset;
             mem = mem + minX * platform.colorBuffer.noChannels;*/

            a1 = a1_;
            a2 = a2_;
            a3 = a3_;

            // This is a very tight loop and need to be optimized heavily even for a suitable framerate
            for (size_t w = minX; w <= maxX; ++w)
            {
                if ((a1 >= 0 && a2 >= 0 && a3 >= 0) ||
                    (a1 <= 0 && a2 <= 0 && a3 <= 0)) // --> Turns on rasterization of anti clockwise triangles
                {

                    // l1 = static_cast<float>(a2) / area;
                    // l2 = static_cast<float>(a3) / area;
                    // l3 = static_cast<float>(a1) / area;

                    //// Z is interpolated linearly in this space, due to perspective division that already occured
                    // float z     = l1 * z0 + l2 * z1 + l3 * z2;

                    // l1          = l1 * inv_w0;
                    // l2          = l2 * inv_w1;
                    // l3          = l3 * inv_w2;

                    // auto  uv    = (l1 * texA + l2 * texB + l3 * texC) * (1.0f / (l1 + l2 + l3));
                    // auto &depth = platform.zBuffer.buffer[h * platform.zBuffer.width + w];

                    if (1)
                    {
                        // Interpolate the depth perspective correctly
                        /*auto rgb = texture.Sample(Vec2(uv), Texture::Interpolation::NEAREST);
                        depth    = z;*/

                        // only calculate mem now if depth test actually passed

                        size_t offset = (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width *
                                        platform.colorBuffer.noChannels;
                        mem    = platform.colorBuffer.buffer + offset + w * platform.colorBuffer.noChannels;

                        mem[0] = 0xFF;
                        mem[1] = 0;
                        mem[2] = 0;
                        mem[3] = 0x00;
                    }
                }
                a1 = a1 + v0.y;
                a2 = a2 + v1.y;
                a3 = a3 + v2.y;
                // mem = mem + 4;
            }
            a1_ = a1_ - v0.x;
            a2_ = a2_ - v1.x;
            a3_ = a3_ - v2.x;
        }
    }
    // }
    // if (Device.Context.ActiveMergeMode == RenderDevice::MergeMode::TEXTURE_MODE)
    //{
    //    // My optimized rasterization
    //    auto texture = Device.Context.GetActiveTexture();
    //    for (size_t h = minY; h <= maxY; ++h)
    //    {
    //        size_t offset =
    //            (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width * platform.colorBuffer.noChannels;
    //        mem = platform.colorBuffer.buffer + offset;
    //        mem = mem + minX * platform.colorBuffer.noChannels;

    //        a   = a_;
    //        // This is a very tight loop and need to be optimized heavily even for a suitable framerate
    //        for (size_t w = minX; w <= maxX; ++w)
    //        {
    //            if (a.compare_f3_greater_eq_than_zero() || a.compare_f3_lesser_eq_than_zero()) // Replace this in
    //            // SIMD version
    //            {
    //                l         = a.swizzle_for_barycentric() * (1.0f / area);
    //                auto z    = l.dot(z_vec);
    //                l         = l * inv_w;
    //                auto  sum = l.dot(SIMD::Vec4ss(1.0f, 1.0f, 1.0f, 0.0f));

    //                float a[4];
    //                _mm_store_ps(a, l.vec);
    //                auto  uv    = (a[3] * texA + a[2] * texB + a[1] * texC) * (1.0f / sum);

    //                /*
    //                auto  uv    = (TexA_simd * a[3] + TexB_simd * a[2] + TexC_simd * a[1]) * (1.0f / sum);
    //                _mm_store_ps(a, uv.vec);*/

    //                auto &depth = platform.zBuffer.buffer[h * platform.zBuffer.width + w];
    //                if (z < depth)
    //                {
    //                    auto rgb = texture.Sample(Vec2(uv), Texture::Interpolation::NEAREST);
    //                    depth    = z;
    //                    mem[0]   = rgb.z;
    //                    mem[1]   = rgb.y;
    //                    mem[2]   = rgb.x;
    //                    mem[3]   = 0x00;
    //                }
    //            }
    //            a   = a + inc_ry;
    //            mem = mem + 4;
    //        }
    //        a_ = a_ - inc_rx;
    //    }
    //}
    // else if (Device.Context.ActiveMergeMode == RenderDevice::MergeMode::COLOR_MODE) // depth mode say
    //{
    //    // TODO :: Tile based rendering
    //    for (size_t h = minY; h <= maxY; ++h)
    //    {
    //        size_t offset =
    //            (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width * platform.colorBuffer.noChannels;
    //        mem = platform.colorBuffer.buffer + offset;
    //        mem = mem + minX * platform.colorBuffer.noChannels;

    //        a   = a_;
    //        // This is a very tight loop and need to be optimized heavily even for a suitable framerate
    //        for (size_t w = minX; w <= maxX; ++w)
    //        {
    //            if (a.compare_f3_greater_eq_than_zero()) // Replace this in SIMD version
    //            {
    //                l           = a.swizzle_for_barycentric() * (1.0f / area);
    //                auto z      = l.dot(z_vec);
    //                l           = l * inv_w;
    //                auto &depth = platform.zBuffer.buffer[h * platform.zBuffer.width + w];
    //                if (z < depth)
    //                {
    //                    depth  = z;
    //                    mem[0] = z * 255;
    //                    mem[1] = z * 255;
    //                    mem[2] = z * 255;
    //                    mem[3] = 0x00;
    //                }
    //            }
    //            a   = a + inc_ry;
    //            mem = mem + 4;
    //        }
    //        a_ = a_ - inc_rx;
    //    }
    //}
    // else
    //{
    //    // Don't do anything here
    //}
}

void ScreenSpace(VertexAttrib3D v0, VertexAttrib3D v1, VertexAttrib3D v2)
{
    // ClipSpace({x1, y1}, {x2, y2}, {x3, y3});
    //
    // It must pass through 2D clipping though .. lets see what happens

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

    // if (z0 == 0.0f || z1 == 0.0f || z2 == 0.0f)
    //    __debugbreak();

    /* Rasteriser(x0, y0, z0, v0.Position.w, x1, y1, z1, v1.Position.w, x2, y2, z2, v2.Position.w, v0.Color, v1.Color,
              v2.Color, v0.TexCoord, v1.TexCoord, v2.TexCoord);*/
    /*ORasteriser(x0, y0, z0, v0.Position.w, x1, y1, z1, v1.Position.w, x2, y2, z2, v2.Position.w, v0.Color, v1.Color,
                v2.Color, v0.TexCoord, v1.TexCoord, v2.TexCoord);*/

    // With raster info struct now
    RasterInfo rs0(x0, y0, z0, v0.Position.w, v0.TexCoord, v0.Color);
    RasterInfo rs1(x1, y1, z1, v1.Position.w, v1.TexCoord, v1.Color);
    RasterInfo rs2(x2, y2, z2, v2.Position.w, v2.TexCoord, v2.Color);

    // Run all threads parallely from here
    // parameterized by rs0,rs1 and rs2
    //// Parallel::ParallelRasteriser(rs0,rs1,rs2);
    //auto &thread_pool = get_current_thread_pool();
    //auto &renderer    = get_current_parallel_renderer();
    // Use the parallel renderer to render all sections in parallel using above thread_pool
    // renderer.parallel_rasterize(thread_pool, rs0, rs1, rs2);
    // wait until all worker thread have been completed
}

void ClipSpace2D(VertexAttrib3D v0, VertexAttrib3D v1, VertexAttrib3D v2, MemAlloc<VertexAttrib3D> const &allocator)
{
    v0.Position = v0.Position.PerspectiveDivide();
    v1.Position = v1.Position.PerspectiveDivide();
    v2.Position = v2.Position.PerspectiveDivide();

    /*std::vector<VertexAttrib3D> outVertices{v0, v1, v2};
    outVertices.reserve(5);

    std::vector<VertexAttrib3D> inVertices{};
    inVertices.reserve(5);*/
    std::vector<VertexAttrib3D, MemAlloc<VertexAttrib3D>> outVertices({v0, v1, v2}, allocator);
    /*outVertices.push_back(v0);
    outVertices.push_back(v1);
    outVertices.push_back(v2);*/
    auto       inVertices = outVertices;

    auto const clipPoly =
        std::vector<Vec2<float32>>{Vec2(-1.0f, 1.0f), Vec2(-1.0f, -1.0f), Vec2(1.0f, -1.0f), Vec2(1.0f, 1.0f)};

    std::vector<Vec2<float32>> clipLines(clipPoly.size());

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
            auto const& v0 = inVertices.at(i), v1 = inVertices.at((i + 1) % inVertices.size());

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
        ScreenSpace(outVertices.at(0), outVertices.at(vertex), outVertices.at((vertex + 1) % outVertices.size()));
    }
}

void Clip3D(VertexAttrib3D v0, VertexAttrib3D v1, VertexAttrib3D v2,
            MemAlloc<Pipeline3D::VertexAttrib3D> const &allocator)
{
    // Before perspective division, clipping would take place
    // Not bothering it to clip against other planes, either take it or reject it fully

    // Code duplication bettter than runtime checks
    // Check against far plane
    if (v0.Position.z > v0.Position.w && v1.Position.z > v1.Position.w && v2.Position.z > v2.Position.w)
        return;
    // Triangle fully rejected
    // Check for right and left boundary
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
    // std::cout << "Triangle does not lie fully outside the clipping boundary" << std::endl;

    // Replace these vectors
    // std::vector<VertexAttrib3D> inVertices{v0, v1, v2};
    // std::vector<VertexAttrib3D> outVertices = inVertices;
    std::vector<VertexAttrib3D, MemAlloc<VertexAttrib3D>> inVertices({v0, v1, v2}, allocator);
    auto                                                  outVertices = inVertices;

    if (v0.Position.z < 0 || v1.Position.z < 0 || v2.Position.z < 0)
    {
        outVertices.clear();
        for (int i = 0; i < inVertices.size(); ++i)
        {
            //  __debugbreak();
            auto const& v0   = inVertices.at(i);
            auto const& v1   = inVertices.at((i + 1) % inVertices.size());
            bool head = v1.Position.z >= 0.0f;
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
                outVertices.push_back(vn);
                if (head)
                    outVertices.push_back(v1);
            }
        }
    }
    for (int vertex = 1; vertex < outVertices.size() - 1; vertex += 1)
    {
        // ScreenSpace(outVertices.at(0), outVertices.at(vertex), outVertices.at((vertex + 1) % outVertices.size()));
        ClipSpace2D(outVertices.at(0), outVertices.at(vertex), outVertices.at((vertex + 1) % outVertices.size()),
                    allocator);
    }
}
// First thing, vertex transform
void Draw(VertexAttrib3D v0, VertexAttrib3D v1, VertexAttrib3D v2, Mat4f const &matrix)
{
    // It will probably transform them into homogenous co-ordinates
    v0.Position = matrix * v0.Position;
    v1.Position = matrix * v1.Position;
    v2.Position = matrix * v2.Position;

    // We are going to clip it against near z plane only
    // Remaining clipping will occur in 2D before Rasterisation stage
    // It saves up from headache of clipping against all six clipping boundaries

    // Clipping most probably occurs before perspective division phase, when all attributes are linear in space

    // Like in OpenGL, depth value will range from 0 to 1
    // Near pixel have depth value of 0 and farther pixel have depth value 1

    // If clipping occurs in homogenous co-ordinates, the projection matrix makes w = positive;
    // Positive w means, anything towards me nearer from near plane, is to be clipped.
    // Clipping against other boundaries won't take place in Homogenous co-ordinates
    // Not sure, why near plane clipping is also being taken place in homogenous system
    // A round through perspective projection matrix again

    // Near plane clipping will introduce one extra triangle at most.
    // Clip3D(v0, v1, v2, Allocator);
}

void Draw(std::vector<VertexAttrib3D> &vertex_vector, std::vector<uint32_t> const &index_vector, Mat4f const &matrix,
          MemAlloc<Pipeline3D::VertexAttrib3D> const &allocator)
{
    assert(index_vector.size() % 3 == 0);

    // TODO :: Parallelize this loop
    // Parallelism should start from here 
    VertexAttrib3D v0, v1, v2;
    // Run the whole pipeline simultaneously on multiple threads

    for (std::size_t i = 0; i < index_vector.size(); i += 3)
    {
        allocator.resource->reset();
        v0          = vertex_vector[index_vector[i]];
        v1          = vertex_vector[index_vector[i + 1]];
        v2          = vertex_vector[index_vector[i + 2]];

        v0.Position = matrix * v0.Position;
        v1.Position = matrix * v1.Position;
        v2.Position = matrix * v2.Position;

        Clip3D(v0, v1, v2, allocator);
    }
}
} // namespace Pipeline3D