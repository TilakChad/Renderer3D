#pragma once

#include "../include/render.h"
#include "../maths/vec.hpp"
#include <vector>

namespace Shape
{

class Shapes
{
};
class Cylinder
{
  public:
    static RenderInfo offload(float radius, float height)
    {
        std::vector<Pipeline3D::VertexAttrib3D> vertices;
        std::vector<uint32_t>                   indices;

        constexpr float                         pi    = 3.141592f;
        float                                   hStep = 0.25f;
        float                                   tStep = 0.25f;

        float                                   h     = 0;
        float                                   t     = 0;
        uint32_t                                index = 0;

        while (h < height)
        {
            for (t = 0; t < 2 * pi; t += tStep)
            {
                Pipeline3D::VertexAttrib3D v0;
                v0.Position = Vec4f(radius * cos(t), h, radius * sin(t), 1.0f);
                v0.Color    = Vec4f(0.4f, 0.4f, 0.4f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f(radius * cos(t + tStep), h, radius * sin(t + tStep), 1.0f);
                // v0.Color    = Vec4f(0.0f, 1.0f, 0.0f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f(radius * cos(t + tStep), h + hStep, radius * sin(t + tStep), 1.0f);
                // v0.Color    = Vec4f(0.0f, 0.0f, 1.0f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f(radius * cos(t), h + hStep, radius * sin(t), 1.0f);
                // v0.Color    = Vec4f(1.0f, 1.0f, 0.0f, 0.0f);
                vertices.push_back(v0);
                indices.insert(indices.end(), {index + 2, index + 1, index, index + 3, index + 2, index});
                index += 4;
            }
            h = h + hStep;
        }
        return RenderInfo(std::move(vertices), std::move(indices), RenderDevice::MergeMode::COLOR_MODE, 0);
    }
};
class Sphere
{
  public:
    static RenderInfo offload(float radius)
    {
        // oof .. wasted too much time here due to incorrect winding order
        std::vector<Pipeline3D::VertexAttrib3D> vertices;
        std::vector<uint32_t>                   indices;

        constexpr float                         pi      = 3.14159265f;
        float                                   phiStep = 0.5f;
        float                                   theStep = 0.5f;

        uint32_t                                index   = 0;

        for (float x = pi / 2; x >= -pi/2; x -= theStep)
        {
            float theta = pi / 2 - x;
            for (float phi = -pi; phi <= pi; phi += phiStep)
            {
                Pipeline3D::VertexAttrib3D v0;
                v0.Position =
                    Vec4f(radius * sin(theta) * cos(phi), radius * cos(theta), radius * sin(theta) * sin(phi), 1.0f);
                v0.Color = Vec4f(0.4f, 0.4f, 0.4f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f(radius * sin(theta - theStep) * cos(phi), radius * cos(theta - theStep),
                                    radius * sin(theta - theStep) * sin(phi), 1.0f);

                // v0.Color    = Vec4f(0.0f, 1.0f, 0.0f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f(radius * sin(theta - theStep) * cos(phi + phiStep), radius * cos(theta - theStep),
                                    radius * sin(theta - theStep) * sin(phi + phiStep), 1.0f);

                // v0.Color    = Vec4f(0.0f, 0.0f, 1.0f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f(radius * sin(theta) * cos(phi + phiStep), radius * cos(theta),
                                    radius * sin(theta) * sin(phi + phiStep), 1.0f);

                // v0.Color    = Vec4f(1.0f, 1.0f, 0.0f, 0.0f);
                vertices.push_back(v0);

                indices.insert(indices.end(), {index, index + 1, index + 2, index + 2, index + 3, index });

                index += 4;
            }
        }

        return RenderInfo(std::move(vertices), std::move(indices), RenderDevice::MergeMode::COLOR_MODE, 0);
    }
};
class Plane
{
};
class Cone
{
  public:
    static RenderInfo offload(float height, float radius)
    {
        std::vector<Pipeline3D::VertexAttrib3D> vertices;
        std::vector<uint32_t>                   indices;

        constexpr float                         pi    = 3.141592f;
        float                                   hStep = 0.25f;
        float                                   tStep = 0.25f;

        float                                   h     = 0;
        float                                   t     = 0;
        uint32_t                                index = 0;

        while (h < height)
        {
            auto z = height - h;
            for (t = 0; t < 2 * pi; t += tStep)
            {
                Pipeline3D::VertexAttrib3D v0;
                v0.Position = Vec4f(z * cos(t), h, z * sin(t), 1.0f);
                v0.Color    = Vec4f(0.4f, 0.4f, 0.4f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f(z * cos(t + tStep), h, z * sin(t + tStep), 1.0f);
                // v0.Color    = Vec4f(0.0f, 1.0f, 0.0f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f((z - hStep) * cos(t + tStep), h + hStep, (z - hStep) * sin(t + tStep), 1.0f);
                // v0.Color    = Vec4f(0.0f, 0.0f, 1.0f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f((z - hStep) * cos(t), h + hStep, (z - hStep) * sin(t), 1.0f);
                // v0.Color    = Vec4f(1.0f, 1.0f, 0.0f, 0.0f);
                vertices.push_back(v0);
                indices.insert(indices.end(), {index + 2, index + 1, index, index + 3, index + 2, index});
                index += 4;
            }
            h = h + hStep;
        }
        return RenderInfo(std::move(vertices), std::move(indices), RenderDevice::MergeMode::COLOR_MODE, 0);
    }
};
} // namespace Shape