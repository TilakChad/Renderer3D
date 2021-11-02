#pragma once 
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    uint8_t *LoadPNGFromFile(const char *image_path, uint32_t *width, uint32_t *height, uint32_t *no_channels,
                             uint32_t *bit_depth);
    uint8_t *LoadPNGFromMemory(uint8_t *buffer, uint32_t length, uint32_t *width, uint32_t *height,
                               uint32_t *no_channels, uint32_t *bit_depth);
    void     DrawImage(const char *img, uint8_t *target_buffer, uint32_t target_width, uint32_t target_height,
                       uint32_t target_channels);

#ifdef __cplusplus
}
#endif
