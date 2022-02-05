#pragma once
#include "platform.h"

#include "../include/texture.hpp"
#include "../maths/vec.hpp"

void ClearColor(uint8_t r, uint8_t g, uint8_t b);
void FastClearColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void RenderBackground(BackgroundTexture const &texture);


// Guess I should write a Math library again first.. Let's do the rasterizer stage first
// First will go with Rasteriser .. later with raytracer

// Hide their definitions
uint32_t CreateTexture(const char *img_path);
uint32_t CreateTextureFromData(Texture &texture);

Texture  GetTexture(uint32_t textureID);
void     SetActiveTexture(uint32_t texture);
Texture  GetActiveTexture();

void     ImageViewer(uint8_t *img_buffer, uint8_t *buffer, uint32_t image_width, uint32_t image_height,
                     uint32_t image_channels, uint32_t buffer_width, uint32_t buffer_height, uint32_t buffer_channels,
                     uint32_t target_width, uint32_t target_height);

namespace Pipeline3D
{
void ClearDepthBuffer();
}