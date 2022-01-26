//// Implement parallel rendering using all the hardware concurrent threads for maximum performance along with SIMD
//// to boost up the fps really high
//#include "../../include/rasteriser.h"
//#include "../../maths/vec.hpp"
//#include "../../utils/thread_pool.h"
//#include <functional>
//// Each thread will process every triangles simultaneously but will render to only their available region
//// No segmented rendering
//
//// For example, for 2 thread working on framebuffer
//// The buffer will be divided into 2 parts and each thread will handle each part individually
//// A thread can operate on each triangle parallely, but that creates invalid color buffer synchronization
//// Each triangle can be operated on parallely, but SIMD is really fine for that stuffs
//namespace Parallel
//{
//
//class ParallelRenderer
//{
//    // Maybe thread pool isn't required here explicitly, if all threads will continuously work on each region
//    // Optimal partition of screen into threads -> To decide
//    // Each time screen buffer changes ParallelRendered need to be remodified
//    // Only the rasteriser stage will be parallelized for now
//    // I guess it should take thread pool as input to initiate parallel operation during Rasterisation
//
//    constexpr static uint32_t no_of_partitions = 4;
//    enum class PartitionType
//    {
//        HORIZONTAL,
//        VERTICAL,
//        EQUAL_GRID
//    };
//    PartitionType parition_type = PartitionType::VERTICAL;
//    // Lets start with equal_grid for now
//
//    // Need bounding box for each grids
//
//    struct BBox
//    {
//        int x0, y0, x1, y1;
//    };
//    BBox     boxes[no_of_partitions];
//    uint32_t boundary[4];
//
//  public:
//    // TODO :: Take the platform and partition the screen space into no_of_partitions
//    ParallelRenderer(ThreadPool const &thread_pool, const int width, const int height)
//    {
//        // Initialize all the bounding boxes
//        // Currently going for equal grid
//        auto w = width / 2;
//        auto h = height / 2;
//        // Four equal sized bounding boxes
//        // which won't get good performance when all triangles are on only one quadrant .. but lets not think about
//        // optimizing that for now
//        boxes[0] = {0, 0, w, h};
//        boxes[1] = {w, 0, width, h};
//        boxes[2] = {w, h, width, height};
//        // Change of idea
//        // We will partition the screen into 4 vertical segments where each thread will work on each thread
//        // It will make checking for y co-ordinates redundant
//        boundary[0] = 0;
//        boundary[1] = width / (no_of_partitions - 1);
//        boundary[2] = 2 * boundary[1];
//        boundary[3] = width;
//    }
//
//    // This program will launch rasteriser on all 4 threads with some preparation
//    // What preparation don't know yet
//    void parallel_rasterize(ThreadPool &thread_pool, Pipeline3D::RasterInfo const &rs0,
//                            Pipeline3D::RasterInfo const &rs1, Pipeline3D::RasterInfo const &rs2)
//    {
//        thread_pool.add_task(std::bind(&std::remove_reference_t<decltype(*this)>::ParallelRasteriser, this,
//                                       std::cref(rs0), std::cref(rs1), std::cref(rs2), 0, boundary[1]));
//        thread_pool.add_task(std::bind(&std::remove_reference_t<decltype(*this)>::ParallelRasteriser, this,
//                                       std::cref(rs0), std::cref(rs1), std::cref(rs2), boundary[1], boundary[2]));
//        thread_pool.add_task(std::bind(&std::remove_reference_t<decltype(*this)>::ParallelRasteriser, this,
//                                       std::cref(rs0), std::cref(rs1), std::cref(rs2), boundary[2], boundary[3]));
//    /*    thread_pool.add_task(std::bind(&std::remove_reference_t<decltype(*this)>::ParallelRasteriser, this,
//                                       std::cref(rs0), std::cref(rs1), std::cref(rs2), 0, 100));
//    */}
//    void ParallelRasteriser(Pipeline3D::RasterInfo const &v0, Pipeline3D::RasterInfo const &v1,
//                            Pipeline3D::RasterInfo const &v2, int32_t XMinBound, int32_t XMaxBound)
//    {
//        Platform platform = GetCurrentPlatform();
//
//        int      minX     = std::min({v0.x, v1.x, v2.x, XMaxBound});
//        int      maxX     = std::max({v0.x, v1.x, v2.x, XMinBound});
//
//        int      minY     = std::min({v0.y, v1.y, v2.y});
//        int      maxY     = std::max({v0.y, v1.y, v2.y});
//
//        // Assume vectors are in clockwise ordering
//        Vec2  p0   = Vec2(v1.x, v1.y) - Vec2(v0.x, v0.y);
//        Vec2  p1   = Vec2(v2.x, v2.y) - Vec2(v1.x, v1.y);
//        Vec2  p2   = Vec2(v0.x, v0.y) - Vec2(v2.x, v2.y);
//
//        Vec2  vec0 = Vec2(v0.x, v0.y);
//        Vec2  vec1 = Vec2(v1.x, v1.y);
//        Vec2  vec2 = Vec2(v2.x, v2.y);
//
//        float area = Vec2<int>::Determinant(p1, p0);
//
//        // Perspective depth interpolation, perspective correct texture interpolation ....
//        Vec2 point = Vec2(minX, minY);
//        // Start collecting points from here
//
//        float a1_ = Vec2<int32_t>::Determinant(point - vec0, p0);
//        float a2_ = Vec2<int32_t>::Determinant(point - vec1, p1);
//        float a3_ = Vec2<int32_t>::Determinant(point - vec2, p2);
//
//        // Read to process 4 pixels at a time
//        SIMD::Vec4ss a1_vec  = SIMD::Vec4ss(a1_, a1_ + p0.y, a1_ + 2 * p0.y, a1_ + 3 * p0.y);
//        SIMD::Vec4ss a2_vec  = SIMD::Vec4ss(a2_, a2_ + p1.y, a2_ + 2 * p1.y, a2_ + 3 * p1.y);
//        SIMD::Vec4ss a3_vec  = SIMD::Vec4ss(a3_, a3_ + p2.y, a3_ + 2 * p2.y, a3_ + 3 * p2.y);
//
//        SIMD::Vec4ss inc_a1  = SIMD::Vec4ss(4 * p0.y);
//        SIMD::Vec4ss inc_a2  = SIMD::Vec4ss(4 * p1.y);
//        SIMD::Vec4ss inc_a3  = SIMD::Vec4ss(4 * p2.y);
//
//        SIMD::Vec4ss inc_a1_ = SIMD::Vec4ss(-p0.x);
//        SIMD::Vec4ss inc_a2_ = SIMD::Vec4ss(-p1.x);
//        SIMD::Vec4ss inc_a3_ = SIMD::Vec4ss(-p2.x);
//
//        SIMD::Vec4ss a1, a2, a3;
//        SIMD::Vec4ss zvec(v0.z, v1.z, v2.z, 0.0f);
//        zvec = zvec * (1.0 / area);
//        // Now processing downwards
//
//        constexpr int hStepSize = 4;
//        for (size_t h = minY; h <= maxY; ++h)
//        {
//            a1 = a1_vec;
//            a2 = a2_vec;
//            a3 = a3_vec;
//
//            for (size_t w = minX; w <= maxX; w += hStepSize)
//            {
//                auto mask = SIMD::Vec4ss::generate_mask(a1, a2, a3);
//                if (mask > 0)
//                {
//
//                    size_t offset = (platform.colorBuffer.height - 1 - h) * platform.colorBuffer.width *
//                                    platform.colorBuffer.noChannels;
//                    uint8_t *off   = platform.colorBuffer.buffer + offset + w * platform.colorBuffer.noChannels;
//                    uint8_t *mem   = off;
//                    auto     depth = &platform.zBuffer.buffer[h * platform.zBuffer.width + w];
//                    using namespace SIMD;
//                    auto   zero = _mm_setzero_ps();
//                    __m128 a    = _mm_unpackhi_ps(a2.vec, a1.vec);
//                    __m128 b    = _mm_unpackhi_ps(zero, a3.vec);
//                    __m128 c    = _mm_unpacklo_ps(a2.vec, a1.vec);
//                    __m128 d    = _mm_unpacklo_ps(zero, a3.vec);
//
//                    // Lol shuffle ni garna parxa tw
//                    auto lvec1 = Vec4ss(_mm_movehl_ps(a, b));
//                    auto lvec2 = Vec4ss(_mm_movelh_ps(b, a));
//
//                    auto lvec3 = Vec4ss(_mm_movehl_ps(c, d));
//                    auto lvec4 = Vec4ss(_mm_movelh_ps(d, c));
//
//                    lvec1      = Vec4ss(_mm_shuffle_ps(lvec1.vec, lvec1.vec, _MM_SHUFFLE(2, 1, 3, 0)));
//                    lvec2      = Vec4ss(_mm_shuffle_ps(lvec2.vec, lvec2.vec, _MM_SHUFFLE(2, 1, 3, 0)));
//                    lvec3      = Vec4ss(_mm_shuffle_ps(lvec3.vec, lvec3.vec, _MM_SHUFFLE(2, 1, 3, 0)));
//                    lvec4      = Vec4ss(_mm_shuffle_ps(lvec4.vec, lvec4.vec, _MM_SHUFFLE(2, 1, 3, 0)));
//                    if (mask & 0x08)
//                    {
//                        // plot first pixel
//                        // calculate z
//
//                        float z = lvec4.dot(zvec);
//                        if (z < depth[0])
//                        {
//                            depth[0] = z;
//                            mem[0]   = z * 255;
//                            mem[1]   = z * 255;
//                            mem[2]   = z * 255;
//                            mem[3]   = 0x00;
//                        }
//                    }
//                    if (mask & 0x04)
//                    {
//                        // plot second pixel
//                        mem     = off + 4;
//
//                        float z = lvec3.dot(zvec);
//                        if (z < depth[1])
//                        {
//                            depth[1] = z;
//
//                            mem[0]   = z * 255;
//                            mem[1]   = z * 255;
//                            mem[2]   = z * 255;
//                            mem[3]   = 0x00;
//                        }
//                    }
//                    if (mask & 0x02)
//                    {
//                        // plot third pixel
//                        mem     = off + 8;
//
//                        float z = lvec2.dot(zvec);
//                        if (z < depth[2])
//                        {
//                            depth[2] = z;
//
//                            mem[0]   = z * 255;
//                            mem[1]   = z * 255;
//                            mem[2]   = z * 255;
//                            mem[3]   = 0x00;
//                        }
//                    }
//                    if (mask & 0x01)
//                    {
//                        // plot fourth pixel
//                        mem     = off + 12;
//
//                        float z = lvec1.dot(zvec);
//                        if (z < depth[3])
//                        {
//                            depth[3] = z;
//                            mem[0]   = z * 255;
//                            mem[1]   = z * 255;
//                            mem[2]   = z * 255;
//                            mem[3]   = 0x00;
//                        }
//                    }
//                }
//
//                a1 = a1 + inc_a1;
//                a2 = a2 + inc_a2;
//                a3 = a3 + inc_a3;
//            }
//            a1_vec = a1_vec + inc_a1_;
//            a2_vec = a2_vec + inc_a2_;
//            a3_vec = a3_vec + inc_a3_;
//        }
//    }
//};
//
//// Try packing all these arguments
//
//} // namespace Parallel