#include "../../include/rasteriser.h"
#include "../../image/PNGLoader.h"
#include "../../maths/vec.hpp"

#include <algorithm>
#include <future>
#include <iostream>
#include <vector>

static RenderDevice Device;

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

void ScreenSpace(VertexAttrib2D v0, VertexAttrib2D v1, VertexAttrib2D v2)
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

void ClipSpace(VertexAttrib2D v0, VertexAttrib2D v1, VertexAttrib2D v2)
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
    std::vector<VertexAttrib2D> outVertices{v0, v1, v2};
    outVertices.reserve(5);

    std::vector<VertexAttrib2D> inVertices{};
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
            auto v0 = inVertices.at(i), v1 = inVertices.at((i + 1) % inVertices.size());

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
                VertexAttrib2D inter;
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

void RenderDevice::Draw(VertexAttrib2D const &v0, VertexAttrib2D const &v1, VertexAttrib2D const &v2)
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

void BresenhamLineRasteriser(VertexAttrib2D const &v0, VertexAttrib2D const &v1)
{
    // Line clipping first
    auto const clipPoly =
        std::vector<Vec2<float32>>{Vec2(-1.0f, 1.0f), Vec2(-1.0f, -1.0f), Vec2(1.0f, -1.0f), Vec2(1.0f, 1.0f)};

    std::vector<Vec2<float32>> clipLines(clipPoly.size());

    for (int i = 0; i < clipPoly.size(); ++i)
        clipLines.at(i) = clipPoly.at((i + 1) % clipPoly.size()) - clipPoly.at(i);

    size_t                      vert = 0;

    std::vector<VertexAttrib2D> outVertices;
    std::vector<VertexAttrib2D> inVertices;

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
            auto v0 = inVertices.at(i), v1 = inVertices.at((i + 1) % inVertices.size());

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

                VertexAttrib2D inter;
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

    auto     v2       = outVertices.back();
    auto     v3       = outVertices.front();

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

void RenderDevice::Draw(VertexAttrib2D const &v0, VertexAttrib2D const &v1)
{
    BresenhamLineRasteriser(v0, v1);
}