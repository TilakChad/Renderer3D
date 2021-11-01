#include "../../include/rasteriser.h"
#include "../../maths/vec.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

void Rasteriser(int x1, int y1, int x2, int y2, int x3, int y3, Vec3<uint8_t> color)
{
    // Its the triangle rasteriser
    // It rasterizes only counter clockwise triangles
    //
    // To make it work for almost every triangle,
    //
    // First comes the clipping phase.. anything outside |1| are clipped against the rectangle
    // Clipping will be done in previous phase

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

    for (int h = minY; h <= maxY; ++h)
    {
        int32_t offset =
            (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width * platform.colorBuffer.noChannels;
        mem = platform.colorBuffer.buffer + offset;
        mem = mem + minX * platform.colorBuffer.noChannels;

        // This is a very tight loop and need to be optimized heavily even for a suitable framerate
        for (int w = minX; w <= maxX; ++w)
        {
            Vec2 point = Vec2(w, h);
            auto a     = Vec2<int>::Determinant(point - vec1, v0) >= 0;
            auto b     = Vec2<int>::Determinant(point - vec2, v1) >= 0;
            auto c     = Vec2<int>::Determinant(point - vec3, v2) >= 0;

            /*if (Vec2<int>::Determinant(point - vec1, v0) >= 0 && Vec2<int>::Determinant(point - vec2, v1) >= 0 &&
                Vec2<int>::Determinant(point - vec3, v2) >= 0)*/
            if ((a && b && c) || (!a && !b && !c))
            {
                mem[0] = color.z;
                mem[1] = color.y;
                mem[2] = color.x;
                mem[3] = 0x00;
            }
            mem = mem + 4;
        }
    }
}

void ScreenSpace(float x1, float y1, float x2, float y2, float x3, float y3, Vec3<uint8_t> color)
{
    // ClipSpace({x1, y1}, {x2, y2}, {x3, y3});
    Platform platform = GetCurrentPlatform();
    int      scrX1    = static_cast<int>((platform.width - 1) / 2 * (x1 + 1));
    int      scrY1    = static_cast<int>((platform.height - 1) / 2 * (1 + y1));

    int      scrX2    = static_cast<int>((platform.width - 1) / 2 * (x2 + 1));
    int      scrY2    = static_cast<int>((platform.height - 1) / 2 * (1 + y2));

    int      scrX3    = static_cast<int>((platform.width - 1) / 2 * (x3 + 1));
    int      scrY3    = static_cast<int>((platform.height - 1) / 2 * (1 + y3));

    Rasteriser(scrX1, scrY1, scrX2, scrY2, scrX3, scrY3, color);
}

// Polygon Clipping using Sutherland-Hodgeman algorithm in NDC space before screen space mapping

void ClipSpace(Vec2<float32> v0, Vec2<float32> v1, Vec2<float32> v2)
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
    std::vector<Vec2<float32>> outVertices{v0, v1, v2};
    outVertices.reserve(5);

    std::vector<Vec2<float32>> inVertices{};
    inVertices.reserve(5);

    auto const clipRect =
        std::vector<Vec2<float32>>{Vec2(-1.0f, 1.0f), Vec2(-1.0f, -1.0f), Vec2(1.0f, -1.0f), Vec2(1.0f, 1.0f)};

    std::vector<Vec2<float32>> clipLines(clipRect.size());

    for (int i = 0; i < clipRect.size(); ++i)
        clipLines.at(i) = clipRect.at((i + 1) % clipRect.size()) - clipRect.at(i);

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

    //        auto intersection = LineIntersect(clipRect[vert], clipRect[vert] + line, v0, v1);
    //        auto point1       = clipRect[vert] + line * intersection.x;
    //        auto point2       = v0 + (v1 - v0) * intersection.y;

    //        if (Vec2<float32>::Determinant(line, v1 - clipRect[vert]) >= 0) // if current point is inside the clip
    //        // line
    //        {
    //            if (Vec2<float32>::Determinant(line, v0 - clipRect[vert]) < 0)
    //            {
    //                outVertices.push_back(point1);
    //            }
    //            outVertices.push_back(v1);
    //        }
    //        else if (Vec2<float32>::Determinant(line, v0 - clipRect[vert]) >= 0)
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
            auto head = Vec2<float>::Determinant(clipRect[vert] + line, v1 - clipRect[vert]) >= 0;
            auto tail = Vec2<float>::Determinant(clipRect[vert] + line, v0 - clipRect[vert]) >= 0;
            if (head && tail)
                outVertices.push_back(v1);
            else if (!(head || tail))
                continue;
            else
            {
                // This edge is divided by the clip line
                auto intersection = LineIntersect(clipRect[vert], clipRect[vert] + line, v0, v1);
                auto point1       = clipRect[vert] + line * intersection.x;
                auto point2       = v0 + (v1 - v0) * intersection.y;
                outVertices.push_back(point1); // Pushes the intersection point
                if (head)
                    outVertices.push_back(v1);
            }
        }
        vert++;
    }
    // std::cout << "By new algo: " << std::endl;
    // for (auto const &i : outVertices)
    //    std::cout << i;
    Vec3<uint8_t> color[] = {Vec3<uint8_t>(0xFF, 0x00, 0x00), Vec3<uint8_t>(0xFF, 0xFF, 0x00),
                             Vec3<uint8_t>(0x00, 0x00, 0xFF)};
    int           col     = 0;
    
    assert(outVertices.size() > 2);
    for (int i = 0; i < outVertices.size(); i += 2)
    {
        ScreenSpace(outVertices.at(i).x, outVertices.at(i).y, outVertices.at(i + 1).x, outVertices.at(i + 1).y,
                    outVertices.at((i + 2) % outVertices.size()).x, outVertices.at((i + 2) % outVertices.size()).y,
                    color[col++]);
    }
}
