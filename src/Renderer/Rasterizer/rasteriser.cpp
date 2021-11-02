#include "../../include/rasteriser.h"
#include "../../maths/vec.hpp"

#include <algorithm>
#include <future>
#include <iostream>
#include <vector>

void Rasteriser(int x1, int y1, int x2, int y2, int x3, int y3, Vec3<uint8_t> attribA, Vec3<uint8_t> attribB,
                Vec3<uint8_t> attribC)
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

    int32_t       area = Vec2<int>::Determinant(v1, v0);

    float         l1 = 0, l2 = 0, l3 = 0; // Barycentric coordinates

    Vec3<uint8_t> resultColor;
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
            auto a1    = Vec2<int>::Determinant(point - vec1, v0);
            auto a2    = Vec2<int>::Determinant(point - vec2, v1);
            auto a3    = Vec2<int>::Determinant(point - vec3, v2);

            if ((a1 >= 0 && a2 >= 0 &&
                 a3 >= 0)) //  || (a1 < 0 && a2 < 0 && a3 < 0)) // --> Turns on rasterization of anti clockwise triangles
            {
                //// Calculate the barycentric coordinates
                // l3          = static_cast<float>(a1) / area;
                // l1          = static_cast<float>(a2) / area;
                // l2          = static_cast<float>(a3) / area;

                // auto color  = l1 * attribA + l2 * attribB + l3 * attribC;
                ////resultColor = Vec3<uint8_t>(color.x, color.y, color.z);
                // mem[0]      = resultColor.z;
                // mem[1]      = resultColor.y;
                // mem[2]      = resultColor.x;
                // mem[3]      = 0x00;

                auto r = (attribA.x * a2 + attribB.x * a3 + attribC.x * a1) / area;
                auto g = (attribA.y * a2 + attribB.y * a3 + attribC.y * a1) / area;
                auto b = (attribA.z * a2 + attribB.z * a3 + attribC.z * a1) / area;
                mem[0] = b;
                mem[1] = g;
                mem[2] = r;
                mem[3] = 0x00;
            }
            mem = mem + 4;
        }
    }

    // Naive direct parallelizatio using libstdc++

    // auto task1 = [=](int minY, int maxY) mutable {
    //    for (int h = minY; h <= maxY; ++h)
    //    {
    //        int32_t offset =
    //            (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width * platform.colorBuffer.noChannels;
    //        mem = platform.colorBuffer.buffer + offset;
    //        mem = mem + minX * platform.colorBuffer.noChannels;

    //        // This is a very tight loop and need to be optimized heavily even for a suitable framerate
    //        for (int w = minX; w <= maxX; ++w)
    //        {
    //            Vec2 point = Vec2(w, h);
    //            auto a     = Vec2<int>::Determinant(point - vec1, v0) >= 0;
    //            auto b     = Vec2<int>::Determinant(point - vec2, v1) >= 0;
    //            auto c     = Vec2<int>::Determinant(point - vec3, v2) >= 0;

    //            /*if (Vec2<int>::Determinant(point - vec1, v0) >= 0 && Vec2<int>::Determinant(point - vec2, v1) >= 0
    //            &&
    //                Vec2<int>::Determinant(point - vec3, v2) >= 0)*/
    //            if ((a && b && c) || (!a && !b && !c))
    //            {
    //                mem[0] = color.z;
    //                mem[1] = color.y;
    //                mem[2] = color.x;
    //                mem[3] = 0x00;
    //            }
    //            mem = mem + 4;
    //        }
    //    }
    //};
    // auto run = std::async(std::launch::async,task1,minY,maxY/3);
    // auto ano = std::async(std::launch::async, task1, maxY / 3 + 1, 2*maxY/3);
    // task1(2 * maxY / 3 + 1, maxY);
    // run.wait();
    // ano.wait();
}

void ScreenSpace(Vec2f v0, Vec2f v1, Vec2f v2, Vec3<uint8_t> attribA, Vec3<uint8_t> attribB, Vec3<uint8_t> attribC)
{
    // ClipSpace({x1, y1}, {x2, y2}, {x3, y3});
    Platform platform = GetCurrentPlatform();
    int      scrX0    = static_cast<int>((platform.width - 1) / 2 * (v0.x + 1));
    int      scrY0    = static_cast<int>((platform.height - 1) / 2 * (1 + v0.y));

    int      scrX1    = static_cast<int>((platform.width - 1) / 2 * (v1.x + 1));
    int      scrY1    = static_cast<int>((platform.height - 1) / 2 * (1 + v1.y));

    int      scrX2    = static_cast<int>((platform.width - 1) / 2 * (v2.x + 1));
    int      scrY2    = static_cast<int>((platform.height - 1) / 2 * (1 + v2.y));

    Rasteriser(scrX0, scrY0, scrX1, scrY1, scrX2, scrY2, attribA, attribB, attribC);
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

            auto head = Vec2<float>::Determinant(clipPoly[vert] + line, v1.Position - clipPoly[vert]) >= 0;
            auto tail = Vec2<float>::Determinant(clipPoly[vert] + line, v0.Position - clipPoly[vert]) >= 0;

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
                inter.TexCoord = Vec2f(0.0f, 0.0f); // Nothing done here .. Might revisit during texture mapping phase

                // TODO -> Handle cases
                auto floatColor = Vec3f(v0.Color.x, v0.Color.y, v0.Color.z) + (inter.Position.x - v0.Position.x) /
                                                                                  (v1.Position.x - v0.Position.x) *
                                                                                  (v1.Color - v0.Color);
                inter.Color = Vec3u8(floatColor.x, floatColor.y, floatColor.z);

                outVertices.push_back(inter); // Pushes the intersection point
                if (head)
                    outVertices.push_back(v1);
            }
        }
        vert++;
    }

    assert(outVertices.size() > 2);
    auto i = 0;
    for (int i = 0; i < outVertices.size()-1; i+=2)
    {
        ScreenSpace(outVertices.at(i).Position, outVertices.at(i + 1).Position,
                    outVertices.at((i + 2) % outVertices.size()).Position, outVertices.at(i).Color,
                    outVertices.at(i + 1).Color, outVertices.at((i + 2)%outVertices.size()).Color);
    }
}
