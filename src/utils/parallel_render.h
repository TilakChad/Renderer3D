#pragma once
#include "../include/rasteriser.h"
#include "../include/render.h"
#include "./thread_pool.h"

namespace Parallel
{
void ParallelTypeErasedDraw(void *arg);
// Lets implement poor man's feedback complete signal

class ParallelRenderer
{
    // Maybe thread pool isn't required here explicitly, if all threads will continuously work on each region
    // Optimal partition of screen into threads -> To decide
    // Each time screen buffer changes ParallelRendered need to be remodified
    // Only the rasteriser stage will be parallelized for now
    // I guess it should take thread pool as input to initiate parallel operation during Rasterisation
    constexpr static uint32_t no_of_partitions = 6; // or number of threads
    enum class PartitionType
    {
        HORIZONTAL,
        VERTICAL,
        EQUAL_GRID
    };
    PartitionType parition_type = PartitionType::VERTICAL;
    // Lets start with equal_grid for now

    // Need bounding box for each grids
    bool completed_rendering[no_of_partitions] = {false};

    struct BBox
    {
        int x0, y0, x1, y1;
    };
    BBox    boxes[no_of_partitions];
    int32_t boundary[no_of_partitions + 1];

  public:
    // Arg address struct
    struct ParallelThreadArgStruct
    {
        // vertex index
        // const std::vector<Pipeline3D::VertexAttrib3D> *vertex_vector;
        // const std::vector<uint32_t>                   *index_vector;
        // const Mat4f                                   *matrix;
        RenderList                           *render_list;
        MemAlloc<Pipeline3D::VertexAttrib3D> *allocator;
        int32_t                               XMinBound;
        int32_t                               XMaxBound;
    };

    ParallelRenderer() = default;
    ParallelRenderer(const int width, const int height);

    // This program will launch rasteriser on all 4 threads with some preparation
    // What preparation don't know yet

    void ParallelPipeline(ThreadPool &thread_pool, std::vector<Pipeline3D::VertexAttrib3D> const &vertex_vector,
                          std::vector<uint32_t> const &index_vector, Mat4f const &matrix,
                          std::vector<MemAlloc<Pipeline3D::VertexAttrib3D>> &allocator);
    void AlternativeParallelPipeline(Alternative::ThreadPool                       &thread_pool,
                                     std::vector<Pipeline3D::VertexAttrib3D> const &vertex_vector,
                                     std::vector<uint32_t> const &index_vector, Mat4f const &matrix,
                                     std::vector<MemAlloc<Pipeline3D::VertexAttrib3D>> &allocator);

    void reset_rendering_status()
    {
        std::fill_n(completed_rendering, 4, false);
    }

    bool done_rendering()
    {
        return std::all_of(completed_rendering, completed_rendering + 4, [](bool x) { return x; });
    }

    void AlternativeParallelRenderablePipeline(Alternative::ThreadPool &thread_pool, RenderList &renderables,
                                               std::vector<MemAlloc<Pipeline3D::VertexAttrib3D>> &allocator);
};
} // namespace Parallel