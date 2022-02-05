#pragma once

#include "../include/texture.hpp"
#include "../maths/matrix.hpp"
#include "../maths/vec.hpp"

#include "../utils/memalloc.h"
#include "renderer.h"
#include <vector>
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



struct RLights // to prevent name clashing with raytracer
{
    Vec4f position; 
    Vec4f color; 
    float intensity; // The colorbuffer doesn't support High Dynamic Range, so intensity have to wait 
};


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
struct RasterInfo
{
    int32_t x;
    int32_t y;
    float   z;
    float   inv_w;
    Vec2f   texCoord;
    Vec4f   color;
    // The position vector to retrieve the fragment position in the screen space required for shading effects
    Vec4f   frag_pos;
};

struct VertexAttrib3D
{
    Vec2f TexCoord;
    Vec4f Position;
    Vec4f Color;
    Vec4f FragPos; 
};
} // namespace Pipeline3D
namespace Pipeline3D
{
void Draw(VertexAttrib3D v0, VertexAttrib3D v1, VertexAttrib3D v2, Mat4f const &matrix);
void Draw(std::vector<VertexAttrib3D> &vertex_vector, std::vector<uint32_t> const &index_vector,
          Mat4f const &transform_matrix, MemAlloc<Pipeline3D::VertexAttrib3D> const &allocator);
} // namespace Pipeline3D

namespace Parallel
{
void ParallelRasteriser(Pipeline3D::RasterInfo const &v0, Pipeline3D::RasterInfo const &v1,
                        Pipeline3D::RasterInfo const &v2);
}