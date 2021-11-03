#pragma once

#include "../maths/vec.hpp"
#include "renderer.h"
// This one is the rasteriser and is responsible for rasterisation operation
// We will start with some simple.. Rasterizing a triangle in a simple fashion

// This rasterizer will only work on triangles for now

struct VertexAttrib2D
{
    Vec2f Position; 
    Vec2f TexCoord;
    Vec3u8 Color; 
};

// void Rasteriser(int x1, int y1, int x2, int y2, int x3, int y3);
// void ScreenSpace(float x1, float y1, float x2, float y2, float x3, float y3);
void ClipSpace(VertexAttrib2D v0, VertexAttrib2D v1, VertexAttrib2D v2);

void SampleTexture(uint8_t *img_buffer, uint8_t *buffer, uint32_t image_width, uint32_t image_height,
                   uint32_t image_channels, uint32_t buffer_width, uint32_t buffer_height, uint32_t buffer_channels,
                   uint32_t target_width, uint32_t target_height);