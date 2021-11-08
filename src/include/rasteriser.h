#pragma once

#include "../include/texture.hpp"
#include "../maths/matrix.hpp"
#include "../maths/vec.hpp"

#include "renderer.h"
// This one is the rasteriser and is responsible for rasterisation operation
// We will start with some simple.. Rasterizing a triangle in a simple fashion

// This rasterizer will only work on triangles for now
namespace Pipeline2D
{

struct VertexAttrib2D
{
    Vec2f Position;
    Vec2f TexCoord;
    Vec4f Color;
};

} // namespace Pipeline2D

// Need seperate interface for 3D pipeline ?
struct RenderDevice // <-- Similiar to DirectX device
{

  public:
    enum class Topology
    {
        TRIANGLES,
        LINES,
        POINTS
    };

    enum class MergeMode
    {
        // These modes are composable with pipe operator, other modes are not
        BLEND_MODE,   // Enables the alpha blending mode
        TEXTURE_MODE, // Enables the texture sampling mode
        COLOR_MODE,   // Just use the interpolated colors
        TEXTURE_BLENDING_MODE,
        NOTHING
    };

    enum class RasteriserMode
    {
        BACK_FACE_CULL,  // Culls the back face, no rendering of Clockwise windings
        FRONT_FACE_CULL, // Culls the front face, don't render Anti-clockwise windings
        NONE_CULL        // Don't cull
    };

    struct CTX
    {
        Topology       ActiveTopology{Topology::TRIANGLES}; // <-- Default topology
        RasteriserMode ActiveRasteriserMode{RasteriserMode::BACK_FACE_CULL};
        MergeMode      ActiveMergeMode{MergeMode::COLOR_MODE};
        Mat4<float>    SceneMatrix{1.0f}; // <-- internal matrix

        uint32_t       ActiveTexture{};

        void           SetPrimitiveTopolgy(Topology topo)
        {
            ActiveTopology = topo;
        }
        void SetMergeMode(MergeMode mode);

        void SetRasteriserMode(RasteriserMode raster)
        {
            ActiveRasteriserMode = raster;
        }

        void SetActiveTexture(uint32_t texture)
        {
            ::SetActiveTexture(texture);
            this->ActiveTexture = texture;
        }

        auto GetActiveTexture()
        {
            return ::GetActiveTexture();
        }

        void SetTransformMatrix(const Mat4f &matrix)
        {
            SceneMatrix = matrix;
        }

    } Context;

    void Draw(Pipeline2D::VertexAttrib2D const &v0, Pipeline2D::VertexAttrib2D const &v1); // <-- Draw lines
    void Draw(Pipeline2D::VertexAttrib2D const &v0, Pipeline2D::VertexAttrib2D const &v1,
              Pipeline2D::VertexAttrib2D const &v2); // <-- Draw triangles

  private:
};

void          ClipSpace(Pipeline2D::VertexAttrib2D v0, Pipeline2D::VertexAttrib2D v1, Pipeline2D::VertexAttrib2D v2);
RenderDevice *GetRasteriserDevice();

namespace Pipeline3D
{
struct VertexAttrib3D
{
    Vec4f Position;
    Vec2f TexCoord;
    Vec4f Color;
};
} // namespace Pipeline3D
namespace Pipeline3D
{
void Draw(VertexAttrib3D v0, VertexAttrib3D v1, VertexAttrib3D v2, Mat4f const &matrix);
}