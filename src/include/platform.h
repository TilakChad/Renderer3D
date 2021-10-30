#pragma once
#include <cstdint>

using float32 = float; 
using float64 = double; 

struct Platform;
typedef void (*SwapBufferFn)(Platform *);

struct Platform
{
	// What should a platform have?
	int32_t height; 
	int32_t width; 
	float32 deltaTime;

	struct
    {
		uint8_t *buffer; 
		int32_t  width; 
		int32_t  height; 
		int8_t   noChannels;
    } colorBuffer;

	SwapBufferFn SwapBuffer;
};

void RendererMainLoop(Platform *platform);