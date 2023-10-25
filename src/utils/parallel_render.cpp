#include "./parallel_render.h"
#include "../include/shader.h"

extern RLights get_light_source();
// Retrieves the current light and eye position
extern Vec3f   get_camera_position();

constexpr bool cast_shadow = false;
// Lets use some template stuffs to control code branching instead of macro definitions
namespace Parallel
{
using namespace Pipeline3D;
static void Rasteriser(Pipeline3D::RasterInfo const &v0, Pipeline3D::RasterInfo const &v1,
                       Pipeline3D::RasterInfo const &v2, int32_t XMinBound, int32_t XMaxBound)
{
    auto            light     = get_light_source();
    auto            cameraPos = get_camera_position();
    constexpr float shiny     = 32.0f;
    // Yup .. Now ready for flat shading
    // the depth map somehow here
    // Next smooth shading
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

    // Calculate the outward normal vector
    Vec3f dir0 = Vec3f(v0.frag_pos);
    Vec3f dir1 = Vec3f(v1.frag_pos);
    Vec3f dir2 = Vec3f(v2.frag_pos);

    // Assuming clockwise ordering we have,
    // Flat shading
    auto normal   = Vec3f::Cross(dir1 - dir0, dir2 - dir1);
    auto centroid = (v0.frag_pos + v1.frag_pos + v2.frag_pos) * (1.0f / 3.0f);
    auto flat     = normal.unit().dot((Vec3f(light.position) - centroid).unit());
    auto shade    = vMax(0.0f, flat) * 0.9f;
    // Not going to implement Gouraud shading -> In Gouraud shading lighting information are calculated at each vertex
    // and barycentric interpolated to each fragment pos
    //
    // Now to Phong Shading and to depth mapping and then close this chapter for maybe, this sem
    // auto eye      = get_camera_position();
    // auto res      = (eye - centroid).unit().dot((Vec3f(light.position) - centroid).unit());

    // Phong shading
    // So basically, in phong shading normals are interpolated along each vertex, since all triangles are flat and their
    // normals are same, we don't need that interpolation here We interpolating frag pos instead and calculate the
    // directional impact, ambient contribution and phong specular on pex pixel basis

    // Now to the depth mapping
    auto lightOrtho = OrthoProjection(-5.0f, 5.0f, -5.0f, 5.0f, -5.0f, 5.0f);
    // clashes with SIMD lookAtMatrix
    auto lightView  = ::lookAtMatrix(light.position, Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f));

    auto shadowPos0 = lightOrtho * lightView * v0.frag_pos;
    auto shadowPos1 = lightOrtho * lightView * v1.frag_pos;
    auto shadowPos2 = lightOrtho * lightView * v2.frag_pos;

    // calculate lightPos
    if (Device->Context.ActiveMergeMode == RenderDevice::MergeMode::COLOR_MODE)
    {
        for (size_t h = minY; h <= maxY; ++h)
        {
            a1 = a1_vec;
            a2 = a2_vec;
            a3 = a3_vec;

            for (size_t w = minX; w <= maxX; w += hStepSize)
            {
                auto mask = SIMD::Vec4ss::generate_nmask(a1, a2, a3);
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
                            // flat shading
                            // rgb      = rgb + (light.color - rgb) * shade;
                            // phong shading

                            // interpolate using barycentric co-ordinate, position of the vertices to find current
                            // fragPos

                            rgb = rgb + (light.color - rgb) * shade;
                            if constexpr (shading == Shading::Phong)
                            {
                                auto pixelPos =
                                    (a[3] * v0.frag_pos + a[2] * v1.frag_pos + a[1] * v2.frag_pos) * (1.0f / bary_sum);
                                // specular constant
                                // reflected vector
                                auto reflect_vec = Vec3f(pixelPos - light.position).unit().reflect(normal).unit();
                                auto specular = powf(vMax(0.0f, (cameraPos - pixelPos).unit().dot(reflect_vec)), shiny);

                                rgb           = rgb + light.color * specular;
                            }

                            depth[0] = z;
                            // mem[0]   = std::clamp(rgb.z * 255,0.0f,1.0f);
                            // mem[1]   = rgb.y * 255;
                            // mem[2]   = rgb.x * 255;
                            // mem[3]   = rgb.w * 255;

                            mem[0] = std::clamp(rgb.z, 0.0f, 1.0f) * 255;
                            mem[1] = std::clamp(rgb.y, 0.0f, 1.0f) * 255;
                            mem[2] = std::clamp(rgb.x, 0.0f, 1.0f) * 255;
                            mem[3] = std::clamp(rgb.w, 0.0f, 1.0f) * 255;
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
                            rgb      = rgb + (light.color - rgb) * shade;

                            if constexpr (shading == Shading::Phong)
                            {

                                auto pixelPos =
                                    (a[3] * v0.frag_pos + a[2] * v1.frag_pos + a[1] * v2.frag_pos) * (1.0f / bary_sum);
                                auto reflect_vec = Vec3f(pixelPos - light.position).unit().reflect(normal).unit();
                                auto specular = powf(vMax(0.0f, (cameraPos - pixelPos).unit().dot(reflect_vec)), shiny);
                                rgb           = rgb + light.color * specular;
                            }
                            depth[1] = z;
                            // mem[0]   = rgb.z * 255;
                            // mem[1]   = rgb.y * 255;
                            // mem[2]   = rgb.x * 255;
                            // mem[3]   = rgb.w * 255;

                            // mem[0]   = std::clamp(rgb.z * 255, 0.0f, 1.0f);
                            // mem[1]   = std::clamp(rgb.y * 255, 0.0f, 1.0f);
                            // mem[2]   = std::clamp(rgb.x * 255, 0.0f, 1.0f);
                            // mem[3]   = std::clamp(rgb.w * 255, 0.0f, 1.0f);

                            mem[0] = std::clamp(rgb.z, 0.0f, 1.0f) * 255;
                            mem[1] = std::clamp(rgb.y, 0.0f, 1.0f) * 255;
                            mem[2] = std::clamp(rgb.x, 0.0f, 1.0f) * 255;
                            mem[3] = std::clamp(rgb.w, 0.0f, 1.0f) * 255;
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
                            rgb      = rgb + (light.color - rgb) * shade;

                            if constexpr (shading == Shading::Phong)
                            {

                                auto pixelPos =
                                    (a[3] * v0.frag_pos + a[2] * v1.frag_pos + a[1] * v2.frag_pos) * (1.0f / bary_sum);
                                auto reflect_vec = Vec3f(pixelPos - light.position).unit().reflect(normal).unit();
                                auto specular = powf(vMax(0.0f, (cameraPos - pixelPos).unit().dot(reflect_vec)), shiny);

                                rgb           = rgb + light.color * specular;
                            }
                            depth[2] = z;
                            // mem[0]   = rgb.z * 255;
                            // mem[1]   = rgb.y * 255;
                            // mem[2]   = rgb.x * 255;
                            // mem[3]   = 0x00;

                            mem[0] = std::clamp(rgb.z, 0.0f, 1.0f) * 255;
                            mem[1] = std::clamp(rgb.y, 0.0f, 1.0f) * 255;
                            mem[2] = std::clamp(rgb.x, 0.0f, 1.0f) * 255;
                            mem[3] = std::clamp(rgb.w, 0.0f, 1.0f) * 255;
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
                            rgb      = rgb + (light.color - rgb) * shade;

                            if constexpr (shading == Shading::Phong)
                            {

                                auto pixelPos =
                                    (a[3] * v0.frag_pos + a[2] * v1.frag_pos + a[1] * v2.frag_pos) * (1.0f / bary_sum);
                                auto reflect_vec = Vec3f(pixelPos - light.position).unit().reflect(normal).unit();
                                auto specular = powf(vMax(0.0f, (cameraPos - pixelPos).unit().dot(reflect_vec)), shiny);

                                rgb           = rgb + light.color * specular;
                            }
                            depth[3] = z;
                            // mem[0]   = rgb.z * 255;
                            // mem[1]   = rgb.y * 255;
                            // mem[2]   = rgb.x * 255;
                            // mem[3]   = 0x00;

                            mem[0] = std::clamp(rgb.z, 0.0f, 1.0f) * 255;
                            mem[1] = std::clamp(rgb.y, 0.0f, 1.0f) * 255;
                            mem[2] = std::clamp(rgb.x, 0.0f, 1.0f) * 255;
                            mem[3] = std::clamp(rgb.w, 0.0f, 1.0f) * 255;
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
        // Do depth mapping for textured floor for now
        auto texture = GetActiveTexture();
        for (size_t h = minY; h <= maxY; ++h)
        {
            a1                   = a1_vec;
            a2                   = a2_vec;
            a3                   = a3_vec;

            constexpr float bias = 0.005f;
            for (size_t w = minX; w <= maxX; w += hStepSize)
            {
                auto mask = SIMD::Vec4ss::generate_nmask(a1, a2, a3);
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
                        auto uv = (a[3] * v0.texCoord + a[2] * v1.texCoord + a[1] * v2.texCoord) * (1.0f / bary_sum);
                        if (z < depth[0])
                        {
                            // sample texture
                            auto rgb = texture.Sample(uv);
                            // Now sample from depth texture
                            // I think that shadow map should be converted first to texture, so that it would be easier
                            // to sample depth value directly from the texture But lets go without it for now Get the
                            // position of the current pixel

                            // auto pixelPos =
                            //  (a[3] * v0.frag_pos + a[2] * v1.frag_pos + a[1] * v2.frag_pos) * (1.0f / bary_sum);
                            // Transform it using the earlier defined ortho + view projection to find its position in
                            // shadow map Lmao .. really? Is depth mapping that much expensive? Need to do per pixel
                            // matrix multiplication? I guess, it would be fine to calculate this transformation once
                            // for each vertex and then do barycentric interpolation along the way

                            // For now, going with matrix transformation

                            // auto posInShadowMap = lightOrtho * lightView * pixelPos;
                            // Ready to burn CPU?
                            // This not enough, now need to sample the depth value at that position

                            // To do that :
                            // Basically, we drew shadow map from the perspective of light which maps the region into
                            // NDC co-ordinates, ignoring depth x and y are mapped to -1 and 1 which needs to be
                            // remapped into the range of 0 to 1 for easy sampling lets not do that remap, and continue
                            // with manual sampling

                            // So take this point and try to retrieve the depth information
                            // Where's emacs artist mode?

                            /*
                                -------------------------------------------------------------------------------
                                |                                                                       (w,h) |
                                |                                                                             |
                                |                               Vertically it ranges from -1 to +1            |
                                |                                   Its same horizontally                     |
                                |                                                                             |
                                |                                                                             |
                                |                                                                             |
                                |                                                                             |
                                |                                                                             |
                                |                                                                             |
                                |                                                                             |
                                |                                                                             |
                                |                                                                             |
                                |                                                                             |
                                |                                                                             |
                                |                                                                             |
                                |   Its inverted due to going with openGL style and how top down bitmap stored|
                                |(0,0)                                                                        |
                                -------------------------------------------------------------------------------


                            */
                            // So take the current obtained point in the range of [-1,1]x[-1,1] and remap to [0,1]
                            // First horizontal mapping
                            // do barycentric interpolation
                            auto posInShadowMap =
                                (a[3] * shadowPos0 + a[2] * shadowPos1 + a[1] * shadowPos2) * (1.0f / bary_sum);

                            auto sampleX = (posInShadowMap.x + 1) / 2.0f;
                            auto sampleY = (posInShadowMap.y + 1) / 2.0f;
                            sampleX      = std::clamp(sampleX, 0.0f, 1.0f);
                            sampleY      = std::clamp(sampleY, 0.0f, 1.0f);
                            // Retrieve the sample at that position
                            uint32_t imgX           = sampleX * (platform.shadowMap.width - 1);
                            uint32_t imgY           = sampleY * (platform.shadowMap.height - 1);

                            float z_from_light_pers = platform.shadowMap.buffer[imgY * platform.shadowMap.width + imgX];
                            // If they are the same point seen directly both by light and the eye, they must have same
                            // depth value
                            depth[0]       = z;
                            auto current_z = std::clamp(posInShadowMap.z, 0.0f, 1.0f);
                            // This calculation can be done incrementally, by calculating first at each vertex and then
                            // incrementally calculating other things
                            auto nshade = shade;
                            if (z_from_light_pers < current_z - bias)
                            {
                                // The current point must be in the shadow, so occlude it
                                // Preferably use shadow correction factor, but its ok
                                nshade = 0.2f;
                            }

                            mem[0] = rgb.z * nshade;
                            mem[1] = rgb.y * nshade;
                            mem[2] = rgb.x * nshade;
                            mem[3] = 0x00;
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
                            auto posInShadowMap =
                                (a[3] * shadowPos0 + a[2] * shadowPos1 + a[1] * shadowPos2) * (1.0f / bary_sum);

                            auto sampleX            = (posInShadowMap.x + 1) / 2.0f;
                            auto sampleY            = (posInShadowMap.y + 1) / 2.0f;
                            sampleX                 = std::clamp(sampleX, 0.0f, 1.0f);
                            sampleY                 = std::clamp(sampleY, 0.0f, 1.0f);
                            uint32_t imgX           = sampleX * (platform.shadowMap.width - 1);
                            uint32_t imgY           = sampleY * (platform.shadowMap.height - 1);

                            float z_from_light_pers = platform.shadowMap.buffer[imgY * platform.shadowMap.width + imgX];
                            // If they are the same point seen directly both by light and the eye, they must have same
                            // depth value
                            auto current_z = std::clamp(posInShadowMap.z, 0.0f, 1.0f);

                            depth[1]       = z;
                            auto nshade    = shade;
                            if (z_from_light_pers < current_z - bias)
                            {
                                nshade = 0.2f;
                            }
                            mem[0] = rgb.z * nshade;
                            mem[1] = rgb.y * nshade;
                            mem[2] = rgb.x * nshade;
                            mem[3] = 0x00;
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
                            auto posInShadowMap =
                                (a[3] * shadowPos0 + a[2] * shadowPos1 + a[1] * shadowPos2) * (1.0f / bary_sum);

                            auto sampleX = (posInShadowMap.x + 1) / 2.0f;
                            auto sampleY = (posInShadowMap.y + 1) / 2.0f;
                            sampleX      = std::clamp(sampleX, 0.0f, 1.0f);
                            sampleY      = std::clamp(sampleY, 0.0f, 1.0f);
                            // Retrieve the sample at that position
                            uint32_t imgX           = sampleX * (platform.shadowMap.width - 1);
                            uint32_t imgY           = sampleY * (platform.shadowMap.height - 1);

                            float z_from_light_pers = platform.shadowMap.buffer[imgY * platform.shadowMap.width + imgX];
                            // If they are the same point seen directly both by light and the eye, they must have same
                            // depth value
                            auto current_z = std::clamp(posInShadowMap.z, 0.0f, 1.0f);

                            depth[2]       = z;
                            auto nshade    = shade;
                            if (z_from_light_pers < current_z - bias)
                                nshade = 0.2f;
                            mem[0] = rgb.z * nshade;
                            mem[1] = rgb.y * nshade;
                            mem[2] = rgb.x * nshade;
                            mem[3] = 0x00;
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
                            auto rgb = texture.Sample(uv);
                            auto posInShadowMap =
                                (a[3] * shadowPos0 + a[2] * shadowPos1 + a[1] * shadowPos2) * (1.0f / bary_sum);

                            auto sampleX = (posInShadowMap.x + 1) / 2.0f;
                            auto sampleY = (posInShadowMap.y + 1) / 2.0f;
                            sampleX      = std::clamp(sampleX, 0.0f, 1.0f);
                            sampleY      = std::clamp(sampleY, 0.0f, 1.0f);
                            // Retrieve the sample at that position
                            uint32_t imgX           = sampleX * (platform.shadowMap.width - 1);
                            uint32_t imgY           = sampleY * (platform.shadowMap.height - 1);

                            float z_from_light_pers = platform.shadowMap.buffer[imgY * platform.shadowMap.width + imgX];
                            // If they are the same point seen directly both by light and the eye, they must have same
                            // depth value
                            auto current_z = std::clamp(posInShadowMap.z, 0.0f, 1.0f);

                            depth[3]       = z;
                            auto nshade    = shade;
                            if (z_from_light_pers < current_z - bias)
                                nshade = 0.2f;
                            mem[0] = rgb.z * nshade;
                            mem[1] = rgb.y * nshade;
                            mem[2] = rgb.x * nshade;
                            mem[3] = 0x00;
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
    RasterInfo rs0(x0, y0, z0, v0.Position.w, v0.TexCoord, v0.Color, v0.FragPos);
    RasterInfo rs1(x1, y1, z1, v1.Position.w, v1.TexCoord, v1.Color, v1.FragPos);
    RasterInfo rs2(x2, y2, z2, v2.Position.w, v2.TexCoord, v2.Color, v2.FragPos);

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
    // The position data should be interpolated and passed before the perspective division phase

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

                // point1 and point2 are same and are the intersection obtained by clipping against the clipping
                // Poly Since they lie on the edge of the triangle, vertex attributes can be linearly
                // interpolated across those two vertices
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

                    inter.FragPos     = (t * v1.FragPos + one_minus_t * v0.FragPos);
                    inter.FragPos     = inter.FragPos * (1.0f / (t + one_minus_t));
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

                    inter.FragPos     = (t * v1.FragPos + one_minus_t * v0.FragPos);
                    inter.FragPos     = inter.FragPos * (1.0f / (t + one_minus_t));
                }
                // Nothing done here .. Might revisit during texture mapping phase
                // TODO -> Handle cases
                // auto floatColor = Vec3f(v0.Color.x, v0.Color.y, v0.Color.z) + (inter.Position.x -
                // v0.Position.x) /
                //                                                                  (v1.Position.x -
                //                                                                  v0.Position.x) * (v1.Color -
                //                                                                  v0.Color);
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
                // MayTODO :: Switch along different interpolation axis .. could be along w
                //__debugbreak();
                float          t  = (-v0.Position.z) / (v1.Position.z - v0.Position.z);
                auto           pn = v0.Position + t * (v1.Position - v0.Position);
                VertexAttrib3D vn;
                vn.Position = pn;

                vn.TexCoord = v0.TexCoord + t * (v1.TexCoord - v0.TexCoord);
                vn.Color    = v0.Color + t * (v1.Color - v0.Color);
                vn.FragPos  = v0.FragPos + t * (v1.FragPos - v0.FragPos);
                outVertices.push_back(vn);
                if (head)
                    outVertices.push_back(v1);
            }
        }
    }
    for (int vertex = 1; vertex < outVertices.size() - 1; vertex += 1)
    {
        Parallel::ClipSpace2D(outVertices.at(0), outVertices.at(vertex),
                              outVertices.at((vertex + 1) % outVertices.size()), allocator, XMinBound, XMaxBound);
    }
}

static void ParallelShadowMapper(RenderList &renderables, MemAlloc<Pipeline3D::VertexAttrib3D> &allocator,
                                 int32_t XMinBound, int32_t XMaxBound)
{
    VertexAttrib3D v0, v1, v2;
    auto           lightOrtho = OrthoProjection(-5.0f, 5.0f, -5.0f, 5.0f, -5.0f, 5.0f);
    // assume light position is directly above the origin, we haves
    auto light     = get_light_source();
    auto lightView = lookAtMatrix(light.position, Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f));
    for (auto const &renderable : renderables.Renderables)
    {
        for (std::size_t i = 0; i < renderable.indices.size(); i += 3)
        {
            allocator.resource->reset();
            v0          = renderable.vertices[renderable.indices[i]];
            v1          = renderable.vertices[renderable.indices[i + 1]];
            v2          = renderable.vertices[renderable.indices[i + 2]];

            v0.Position = lightOrtho * lightView * renderable.model_transform * v0.Position;
            v1.Position = lightOrtho * lightView * renderable.model_transform * v1.Position;
            v2.Position = lightOrtho * lightView * renderable.model_transform * v2.Position;

            ShadowMapper::Clip3D(v0, v1, v2, allocator, XMinBound, XMaxBound);
        }
    }
}

void ParallelTypeErasedShadow(void *arg)
{
    // Retrieve back the type erased information
    auto drawArgs = static_cast<Parallel::ParallelRenderer::ParallelThreadArgStruct *>(arg);
    /*ParallelDraw(*drawArgs->vertex_vector, *drawArgs->index_vector, *drawArgs->matrix, *drawArgs->allocator,
                 drawArgs->XMinBound, drawArgs->XMaxBound);*/
    ParallelShadowMapper(*drawArgs->render_list, *drawArgs->allocator, drawArgs->XMinBound, drawArgs->XMaxBound);
}


static void ParallelRenderableDraw(RenderList &renderables, MemAlloc<Pipeline3D::VertexAttrib3D> &allocator,
                                   int32_t XMinBound, int32_t XMaxBound)
{
    VertexAttrib3D v0, v1, v2;
    // Run the whole pipeline simultaneously on multiple threads
    auto device = GetRasteriserDevice();
    // For depth mapping, it should be made 2 pass rendering ..
    // We can call 2 pass on per triangle basis or as a whole
    // Lets try the whole pipeline method first

    // Generate shadow maps
    //// set up orthographic projection matrix that covers the square field of dimension 5 x 5

    // auto lightOrtho = OrthoProjection(-5.0f, 5.0f, -5.0f, 5.0f, -5.0f, 5.0f);
    //// assume light position is directly above the origin, we have
    // auto light     = get_light_source();
    // auto lightView = lookAtMatrix(light.position, Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f));
    // for (auto const &renderable : renderables.Renderables)
    //{
    //     for (std::size_t i = 0; i < renderable.indices.size(); i += 3)
    //     {
    //         allocator.resource->reset();
    //         v0          = renderable.vertices[renderable.indices[i]];
    //         v1          = renderable.vertices[renderable.indices[i + 1]];
    //         v2          = renderable.vertices[renderable.indices[i + 2]];

    //        v0.Position = lightOrtho * lightView * renderable.model_transform * v0.Position;
    //        v1.Position = lightOrtho * lightView * renderable.model_transform * v1.Position;
    //        v2.Position = lightOrtho * lightView * renderable.model_transform * v2.Position;

    //        ShadowMapper::Clip3D(v0, v1, v2, allocator, XMinBound, XMaxBound);
    //    }
    //}

    for (auto const &renderable : renderables.Renderables)
    {
        device->Context.ActiveMergeMode = renderable.merge_mode;
        if (renderable.merge_mode == RenderDevice::MergeMode::TEXTURE_MODE)
            SetActiveTexture(renderable.textureID);
        assert(renderable.indices.size() % 3 == 0);
        // First step rendering

        for (std::size_t i = 0; i < renderable.indices.size(); i += 3)
        {
            allocator.resource->reset();
            v0 = renderable.vertices[renderable.indices[i]];
            v1 = renderable.vertices[renderable.indices[i + 1]];
            v2 = renderable.vertices[renderable.indices[i + 2]];

            // Allow passing of model and perspective matrix seperately
            v0.FragPos = renderable.model_transform * v0.Position;
            v1.FragPos = renderable.model_transform * v1.Position;
            v2.FragPos = renderable.model_transform * v2.Position;
            // Am I an idiot? -> Yes, apparently I am.

            v0.Position = renderable.scene_transform * renderable.model_transform * v0.Position;
            v1.Position = renderable.scene_transform * renderable.model_transform * v1.Position;
            v2.Position = renderable.scene_transform * renderable.model_transform * v2.Position;

            // Only use the model transform to transform the fragPos vectors ... They aren't subjected to
            // perspective projection Nature doesn't work depending on how our eyes perceive the effect .. Its
            // absolute

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
        // task.task      = Alternative::ThreadPool::ThreadPoolFunc(Parallel::ParallelTypeErasedDraw, &args[count++]);
        task.task      = Alternative::ThreadPool::ThreadPoolFunc(Parallel::ParallelTypeErasedShadow, &args[count++]);
    }
    // Every thread must wait until the generation of the shadow mapping

    thread_pool.started(&waiter);
    thread_pool.wait_till_finished();

    std::latch newwaiter(no_of_partitions);
    count = 0u;
    for (auto &task : *thread_pool.get_passive_ptr())
    {
        /*task = Alternative::ThreadPool::AlternativeTaskDesc{
            false, Alternative::ThreadPool::ThreadPoolFunc(Parallel::ParallelTypeErasedDraw, &args[count++])};*/
        task.completed = false;
        task.task      = Alternative::ThreadPool::ThreadPoolFunc(Parallel::ParallelTypeErasedDraw, &args[count++]);
    }
    thread_pool.started(&newwaiter);
    thread_pool.wait_till_finished();
}

} // namespace Parallel

namespace ShadowMapper
{
using namespace Pipeline3D;
// woaahhh .. need to duplicate clip3d, screen space, and clip2d also

static void ShadowMappingRasteriser(Pipeline3D::RasterInfo const &v0, Pipeline3D::RasterInfo const &v1,
                                    Pipeline3D::RasterInfo const &v2, int32_t XMinBound, int32_t XMaxBound)
{
    // This rasteriser will only map depth values, nothing else
    Platform platform = GetCurrentPlatform();

    auto     Device   = GetRasteriserDevice();
    int32_t  minX     = vMax(vMin(v0.x, v1.x, v2.x), XMinBound);
    int32_t  maxX     = vMin(vMax(v0.x, v1.x, v2.x), XMaxBound);

    int      minY     = vMin(v0.y, v1.y, v2.y);
    int      maxY     = vMax(v0.y, v1.y, v2.y);

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

    for (size_t h = minY; h <= maxY; ++h)
    {
        a1 = a1_vec;
        a2 = a2_vec;
        a3 = a3_vec;

        for (size_t w = minX; w <= maxX; w += hStepSize)
        {
            auto mask = SIMD::Vec4ss::generate_nmask(a1, a2, a3);
            if (mask > 0)
            {
                auto depth = &platform.shadowMap.buffer[h * platform.shadowMap.width + w];
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
                    float z = lvec1.dot(zvec);
                    if (z < depth[0])
                        depth[0] = z;
                }

                if (mask & 0x04)
                {
                    float z = lvec2.dot(zvec);
                    if (z < depth[1])
                        depth[1] = z;
                }
                if (mask & 0x02)
                {
                    float z = lvec3.dot(zvec);
                    if (z < depth[2])
                        depth[2] = z;
                }
                if (mask & 0x01)
                {
                    // plot fourth pixel
                    float z = lvec4.dot(zvec);
                    if (z < depth[3])
                        depth[3] = z;
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
    RasterInfo rs0(x0, y0, z0, v0.Position.w, v0.TexCoord, v0.Color, v0.FragPos);
    RasterInfo rs1(x1, y1, z1, v1.Position.w, v1.TexCoord, v1.Color, v1.FragPos);
    RasterInfo rs2(x2, y2, z2, v2.Position.w, v2.TexCoord, v2.Color, v2.FragPos);

    ShadowMapper::ShadowMappingRasteriser(rs0, rs1, rs2, XMinBound, XMaxBound);
}

static void ClipSpace2D(VertexAttrib3D v0, VertexAttrib3D v1, VertexAttrib3D v2, MemAlloc<VertexAttrib3D> &allocator,
                        int32_t XMinBound, int32_t XMaxBound)
{
    // The position data should be interpolated and passed before the perspective division phase

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

                // point1 and point2 are same and are the intersection obtained by clipping against the clipping
                // Poly Since they lie on the edge of the triangle, vertex attributes can be linearly
                // interpolated across those two vertices
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
                }
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
        ShadowMapper::ScreenSpace(outVertices.at(0), outVertices.at(vertex),
                                  outVertices.at((vertex + 1) % outVertices.size()), XMinBound, XMaxBound);
    }
}

void Clip3D(VertexAttrib3D const &v0, VertexAttrib3D const &v1, VertexAttrib3D const &v2,
                   MemAlloc<Pipeline3D::VertexAttrib3D> &allocator, int32_t XMinBound, int32_t XMaxBound)
{
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
                // MayTODO :: Switch along different interpolation axis .. could be along w
                //__debugbreak();
                float          t  = (-v0.Position.z) / (v1.Position.z - v0.Position.z);
                auto           pn = v0.Position + t * (v1.Position - v0.Position);
                VertexAttrib3D vn;
                vn.Position = pn;

                outVertices.push_back(vn);
                if (head)
                    outVertices.push_back(v1);
            }
        }
    }
    for (int vertex = 1; vertex < outVertices.size() - 1; vertex += 1)
    {
        ShadowMapper::ClipSpace2D(outVertices.at(0), outVertices.at(vertex),
                                  outVertices.at((vertex + 1) % outVertices.size()), allocator, XMinBound, XMaxBound);
    }
}

} // namespace ShadowMapper
