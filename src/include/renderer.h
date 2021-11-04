#pragma once
#include "platform.h"

#include "../maths/vec.hpp"

void ClearColor(uint8_t r, uint8_t g, uint8_t b);
void FastClearColor(uint8_t r, uint8_t g, uint8_t b);

// Guess I should write a Math library again first.. Let's do the rasterizer stage first
// First will go with Rasteriser .. later with raytracer

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

    Vec3u8 Sample(Vec2f uv, Interpolation type = Interpolation::NEAREST);
};

uint32_t CreateTexture(const char *img_path);
Texture  GetTexture(uint32_t textureID);
void     SetActiveTexture(uint32_t texture);
Texture  GetActiveTexture();

void     ImageViewer(uint8_t *img_buffer, uint8_t *buffer, uint32_t image_width, uint32_t image_height,
                     uint32_t image_channels, uint32_t buffer_width, uint32_t buffer_height, uint32_t buffer_channels,
                     uint32_t target_width, uint32_t target_height);