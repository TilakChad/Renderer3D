#include "../include/renderer.h"
#include "../image/PNGLoader.h"

#include <cstring>
#include <vector>
#include <algorithm>

// Where to put these info? .. Both rasteriser and ray tracer would need this info though
// Trying to implement a common abstraction for both rasterizer and raytracer

// Some hacks for not exporting these functions to outside caller

static std::vector<Texture> Textures;
static uint32_t             ActiveTexture;

void                        ClearColor(uint8_t r, uint8_t g, uint8_t b)
{
    Platform platform = GetCurrentPlatform();
    uint8_t *mem      = platform.colorBuffer.buffer;
    for (int i = 0; i < platform.colorBuffer.height; ++i)
    {
        for (int j = 0; j < platform.colorBuffer.width; ++j)
        {
            *mem++ = b;
            *mem++ = g;
            *mem++ = r;
            mem++;
        }
    }
}

void FastClearColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    // Noice this approach is around 15 times faster than the non optimized one
    // Might write this in assembly lol
    // Will be practice on assembly too
    Platform platform = GetCurrentPlatform();
    uint8_t *mem      = platform.colorBuffer.buffer;
    for (int i = 0; i < platform.colorBuffer.width; ++i)
    {
        *mem++ = b * a / 255.0f;
        *mem++ = g * a / 255.0f;
        *mem++ = r * a / 255.0f;
        *mem++ = a;
    }
    mem             = platform.colorBuffer.buffer;

    uint32_t filled = 1;
    uint32_t stride = platform.colorBuffer.width * platform.colorBuffer.noChannels;

    // __debugbreak();
    while (filled < platform.colorBuffer.height)
    {
        uint32_t remaining = platform.colorBuffer.height - filled;
        if (filled < remaining)
        {
            memcpy((void *)(mem + filled * stride), mem, filled * stride);
            filled = filled * 2;
        }
        else
        {
            memcpy((void *)(mem + filled * stride), mem, remaining * stride);
            filled = filled + remaining;
        }
    }
}

void SetBackgroundImage(std::string_view image_path)
{
    // TODO :: Cache this image 
    Texture tex; 
    tex.raw_data = LoadPNGFromFile(image_path.data(), &tex.width, &tex.height, &tex.channels, &tex.bit_depth);

}

uint32_t CreateTexture(const char *img_path) // It returns handle to that texture
{
    Texture tex;
    tex.raw_data = LoadPNGFromFile(img_path, &tex.width, &tex.height, &tex.channels, &tex.bit_depth);
    if (!tex.raw_data)
        return 0;
    tex.bValid    = true;
    tex.textureID = Textures.size() + 1;
    Textures.push_back(tex);
    return tex.textureID;
}

uint32_t CreateTextureFromData(Texture &texture)
{
    texture.bValid = true; 
    texture.textureID = Textures.size() + 1; 
    Textures.push_back(texture); 
    return texture.textureID;
}

Texture GetTexture(uint32_t textureID)
{
    assert(textureID <= Textures.size());
    assert(textureID != 0);
    return Textures.at(textureID - 1);
}

void SetActiveTexture(uint32_t texture)
{
    ActiveTexture = texture;
}

Texture GetActiveTexture()
{
    return Textures.at(ActiveTexture - 1);
}

void ImageViewer(uint8_t *img_buffer, uint8_t *buffer, uint32_t image_width, uint32_t image_height,
                 uint32_t image_channels, uint32_t buffer_width, uint32_t buffer_height, uint32_t buffer_channels,
                 uint32_t target_width, uint32_t target_height)
{
    // This is the basic image viewer .. Now should I add panning? or not?
    // Guess, not for now. I need to do texture mapping first
    uint32_t xoff = 0, yoff = 0;
    uint32_t xfill = 0, yfill = 0;
    uint32_t ximg = 0, yimg = 0;

    // defaults

    xfill = target_width;
    yfill = target_height;
    xoff  = (buffer_width - target_width) / 2;
    yoff  = (buffer_height - target_height) / 2;
    ximg  = 0;
    yimg  = 0;

    if (target_width > buffer_width || target_height > buffer_height) // --> This case not handled for now
    {
        // Calculate the portion of the image that is visible in the current screen
        //   return;
        if (target_width > buffer_width)
        {
            xoff  = 0;
            ximg  = (target_width - buffer_width) / 2;
            xfill = buffer_width - 1;
        }
        if (target_height > buffer_height)
        {
            yoff  = 0;
            yimg  = (target_height - buffer_height) / 2;
            yfill = buffer_height - 1;
        }
    }

    uint8_t *img = img_buffer;
    // For each pixel, do inverse transformation from image space to object space
    auto mapToImg = [=](uint32_t x, uint32_t y) {
        auto h = target_height, w = target_width;
        auto H = image_height, W = image_width;
        return Vec2<uint32_t>(x * H / h, y * W / w);
    };

    int stride = buffer_width * buffer_channels;

    for (int h = yoff; h < yfill + yoff; ++h)
    {
        img = buffer + h * stride + xoff * buffer_channels;
        for (int w = xoff; w < xfill + xoff; ++w)
        {
            auto pos   = mapToImg(yimg + h - yoff, ximg + w - xoff); // -> Retrieve image at position h row w column
            auto pixel = img_buffer + (pos.x * image_width + pos.y) * image_channels;
            *img++     = pixel[2];
            *img++     = pixel[1];
            *img++     = pixel[0];
            img++;
        }
    }
}

void RenderBackground(BackgroundTexture const& texture)
{
    auto platform = GetCurrentPlatform();
    std::memcpy(platform.colorBuffer.buffer, texture.raw_bckg_data,
                sizeof(uint8_t) * platform.colorBuffer.width * platform.colorBuffer.height *
                    platform.colorBuffer.noChannels);
}

namespace Pipeline3D
{

void ClearDepthBuffer()
{
    // I guess setting it to zero would be better though ... since its taking a long time too 
    auto platform = GetCurrentPlatform();
    /*for (uint32_t h = 0; h < platform.zBuffer.height; ++h)
    {
        for (uint32_t w = 0; w < platform.zBuffer.width; ++w)
        {
            platform.zBuffer.buffer[h * platform.zBuffer.width + w] = 1.0f;
        }
    }*/

    // who said std:: algorithms are bad :D
    auto size = platform.zBuffer.width * platform.zBuffer.height; 
    std::fill(platform.zBuffer.buffer,platform.zBuffer.buffer + size, 1.0f); 
    std::fill(platform.shadowMap.buffer, platform.shadowMap.buffer + platform.shadowMap.width * platform.zBuffer.height,
              1.0f);
    // std::memset(platform.zBuffer.buffer, 0, sizeof(float) * size);
}
// SetBackGround 
   
} // namespace Pipelin3D