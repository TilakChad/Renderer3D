#pragma once

#include "../image/PNGLoader.h"
#include "../maths/vec.hpp"
#include "./platform.h"
#include <cstdint>
#include <format>

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
        static inline struct GaussianBlur
        {
            constexpr static float32 kernel[] = {1.0f / 16, 2.0f / 16, 1.0f / 16, 2.0f / 16, 4.0f / 16,
                                                 2.0f / 16, 1.0f / 16, 2.0f / 16, 1.0f / 16}; // --> Gaussian Blur
        } GaussianBlur;
        static inline struct Sharpen
        {
            constexpr static float32 kernel[] = {0, -1, 0, -1, 5, -1, 0, -1, 0}; // --> Sharpen
        } Sharpen;
        static inline struct EdgeDetection
        {
            constexpr static float32 kernel[] = {-1, -1, -1, -1, 8, -1, -1, -1, -1};
        } EdgeDetection;
        constexpr static Vec2<int16_t> Sampler[] = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {0, 0},
                                                    {1, 0},   {-1, 1}, {0, 1},  {1, 1}};
    };

    Vec3u8 Sample(Vec2f uv, Interpolation type = Interpolation::NEAREST);

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
template <typename T> inline uint8_t *Texture::Convolve(T kernel) requires requires
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

// Naive updating of Background Texture

struct BackgroundTexture
{
    // Frame data
    // Do nearest neighborhood interpolation to swap out the color buffer,
    Texture  texture;

    uint32_t cbuffer_width;
    uint32_t cbuffer_height;
    uint32_t cbuffer_channels;

    uint8_t *raw_bckg_data = nullptr;

    void     CreateBackgroundTexture(std::string_view image_path)
    {
        texture.raw_data =
            LoadPNGFromFile(image_path.data(), &texture.width, &texture.height, &texture.channels, &texture.bit_depth);
        std::cerr << std::format("Loaded PNG {}, width : {}, height :{}\n", image_path.data(), texture.width,
                                 texture.height)
                  << std::endl;
    }

    void SampleForCurrentFrameBuffer(Platform *platform, bool applyGaussianBlur)
    {
        if (raw_bckg_data)
            delete[] raw_bckg_data;

        cbuffer_channels    = platform->colorBuffer.noChannels;
        cbuffer_width       = platform->colorBuffer.width;
        cbuffer_height      = platform->colorBuffer.height;

        uint8_t *sampleData = texture.raw_data;
        raw_bckg_data       = new uint8_t[cbuffer_width * cbuffer_height * cbuffer_channels];

        // Now sample the image from above texture using Nearest Interpolation
        auto mem = raw_bckg_data;
        for (int h = 0; h < cbuffer_height; ++h)
        {
            uint32_t y = h * texture.height / cbuffer_height;
            for (int w = 0; w < cbuffer_width; ++w)
            {
                // Simple linear mapping
                uint32_t x = w * texture.width / cbuffer_width;
                // std::memcpy(mem, texture.raw_data + (y * texture.width + x) * texture.channels, sizeof(uint8_t) *
                // cbuffer_channels);
                auto raw = sampleData + (y * texture.width + x) * texture.channels;
                mem[0]   = raw[2];
                mem[1]   = raw[1];
                mem[2]   = raw[0];
		if (texture.channels == 3)
			mem[3] = 0xFF;
		else 
			mem[3] = raw[3];

                mem += cbuffer_channels;
            }
        }
	printf("Number of channels; %d\n", texture.channels);

        if (applyGaussianBlur)
        {
            // use the result above to apply gaussian blue several times
            Texture blur;
            blur.bit_depth              = texture.bit_depth;
            blur.channels               = cbuffer_channels;
            blur.width                  = cbuffer_width;
            blur.height                 = cbuffer_height;
            blur.raw_data               = raw_bckg_data;
            constexpr int GaussianCount = 20;
            for (int times = 0; times < GaussianCount; ++times)
            {
                auto newblur = blur.Convolve(Texture::Convolution::GaussianBlur);
                delete[] blur.raw_data; 
                blur.raw_data = newblur; 
            }
            raw_bckg_data = blur.raw_data;
        }
    }
};
