#include "../include/texture.hpp"
#include "../include/renderer.h"

Vec3u8 Texture::Sample(Vec2f uv, Interpolation type)
{
    // ignore type for now
    // Just sampling part
    // Texture sampling is similar to that of OpenGL, not like that of DirectX
    // PNGs are loaded in top-down order PNG loader

    /*

  (0,1)____________________ (1,1)
       |                   |
       |                   |
       |				   |
       |				   |
       |                   |
       |                   |
       |                   |
       |___________________|
   (0,0)                  (1,0)

    */
    // Giving sample textured pixel is quite simple

    /*
        x : 0 -> 0,            1 -> image_width  .. There's no image_width column .. only image_width - 1 column
        y : 0 -> image_height, 1 -> 0            .. Same, no image_height row, only image_height - 1 row
    */

    if (type == Interpolation::NEAREST)
    {

        uint32_t xpos, ypos;
        xpos = static_cast<int>(uv.x * (this->width - 1));
        ypos = static_cast<int>((1 - uv.y) * (this->height - 1));

        // Provide pixel in order r,g,b
        assert(raw_data != nullptr);

        uint8_t *pixel = raw_data + (ypos * this->width + xpos) * this->channels;
        Vec3u8   pixel_color;
        if (this->channels == 1)
            pixel_color = Vec3u8(pixel[0]);
        else
            pixel_color = Vec3u8(pixel[0], pixel[1], pixel[2]);
        return pixel_color;
    }

    // Assuming type is linear, we have
    float32 xpos, ypos;
    xpos = (uv.x * (this->width - 1));
    ypos = (1 - uv.y) * (this->height - 1);
    if ((uint32_t)xpos == this->width - 1)
    {
        // return nearest neighborhood
        uint8_t *pixel = raw_data + ((uint32_t)ypos * this->width + (uint32_t)xpos) * this->channels;
        return Vec3u8(pixel[0], pixel[1], pixel[2]);
    }
    else
    {
        // Perform linear sampling
        // Linear sampling is done along only one axis and is linear interpolation between two nearby pixels
        uint32_t x      = static_cast<uint32_t>(xpos);
        float32  xfrac  = xpos - x;
        uint8_t *pixel  = raw_data + ((uint32_t)ypos * this->width + (uint32_t)xpos) * this->channels;

        auto     cColor = Vec3u8(pixel[0], pixel[1], pixel[2]); // --> Current pixel
        pixel           = pixel + this->channels;

        auto nColor     = Vec3u8(pixel[0], pixel[1], pixel[2]); // -->Next pixel on the row <-- Guarranted to exist

        // auto fColor     = nColor - cColor; // <-- signed overflow/ unsigned wraparound
        auto fColor =
            Vec3<int16_t>((int16_t)nColor.x - cColor.x, (int16_t)nColor.y - cColor.y, (int16_t)nColor.z - cColor.z);
        auto rColor = xfrac * fColor;

        return cColor + Vec3u8(rColor.x, rColor.y, rColor.z);
    }

    // Bilinear sampling .. next time
    return {};
}
