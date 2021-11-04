#pragma once

#include "../maths/vec.hpp"
#include <cstdint>

struct Texture
{
    uint8_t *raw_data;
    bool     bValid = false;

    uint32_t textureID; // may not be necessary though

    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t bit_depth;

    enum class Interpolation
    {
        NEAREST,
        LINEAR,
        BILINEAR
    };

    struct Convolution
    {
        static inline struct
        {
            constexpr static float32 kernel[] = {1.0f / 16, 2.0f / 16, 1.0f / 16, 2.0f / 16, 4.0f / 16,
                                                 2.0f / 16, 1.0f / 16, 2.0f / 16, 1.0f / 16}; // --> Gaussian Blur
        } GaussianBlur;
        static inline struct
        {
            constexpr static float32 kernel[] = {0, -1, 0, -1, 5, -1, 0, -1, 0}; // --> Sharpen
        } Sharpen;
        static inline struct
        {
            constexpr static float32 kernel[] = {-1, -1, -1, -1, 8, -1, -1, -1, -1};
        } EdgeDetection;
        constexpr static Vec2<int16_t> Sampler[] = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {0, 0},
                                                    {1, 0},   {-1, 1}, {0, 1},  {1, 1}};
    };

    Vec3u8   Sample(Vec2f uv, Interpolation type = Interpolation::NEAREST);

    // <-- Returns the Gaussian Blurred image of the currently holding texture .. Will allocate

    // Its one of the C++20's fancy features
    // Lol requires need to be repeated in both declaration and definition
    template <typename T> uint8_t *Convolve(T kernel) requires requires
    {
        kernel.kernel;
    };

    void ViewImage(uint8_t *buffer, uint32_t buffer_width, uint32_t buffer_height, uint32_t buffer_channels,
                   uint32_t target_width, uint32_t target_height);
};


// Lmao .. static linkage fiasco ... This method is unique but ...
template <typename T> 
inline uint8_t *Texture::Convolve(T kernel) requires requires
{
    kernel.kernel;
}
{
    auto     kern     = kernel.kernel;
    uint8_t *gaussian = new uint8_t[width * height * channels];
    // We gonna use ranges zip
    // auto pair = std::ranges::zip .. lol not supported till

    auto clamp = [=](int16_t x) {
        x = x > 255 ? 255 : x;
        return x < 0 ? (uint8_t)0 : (uint8_t)x;
    };

    uint8_t *pixel;
    for (int h = 1; h < height - 1; ++h)
    {
        for (int w = 1; w < width - 1; ++w)
        {
            // Do one dimensional Gaussian blur first ? or what?
            // No, do 2D Gaussian Blur first
            Vec3f color{};
            for (int k = 0; k < 9; ++k)
            {
                uint32_t x, y;
                x            = Convolution::Sampler[k].x + w;
                y            = Convolution::Sampler[k].y + h;
                pixel        = raw_data + (y * this->width + x) * channels;
                Vec3f pColor = Vec3f(pixel[0], pixel[1], pixel[2]);
                color        = color + kern[k] * pColor;
            }
            pixel    = gaussian + (width * h + w) * channels;

            pixel[0] = clamp(color.x);
            pixel[1] = clamp(color.y);
            pixel[2] = clamp(color.z);
        }
    }
    return gaussian;
}
