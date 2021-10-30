#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN 
#define WIN32_LEAN_AND_MEAN 
#endif 

#ifndef UNICODE 
#define UNICODE 
#endif

#include <Windows.h>
#include "../include/platform.h"

#define RENDERER_WIN32_CLASS_NAME L"RendererClass"
// Copying most of the things from GLFW .. What else to do, me knows nothing else 

struct Win32Window
{
	HWND	handle;
	HICON	icon; 
	bool	cursorTracked; 
	bool	frameAction;
	bool	maximized; 

	bool	transparent; 

	UINT32	width, height; 
	INT32	lastCurPosX, lastCurPosY; 

	struct 
	{
        BITMAPINFOHEADER bmpInfoHeader;
    } renderBuffer;

	struct 
	{
        LARGE_INTEGER frequency; 
		LARGE_INTEGER counter; 
	} timer;
};

bool InitPlatform(void);
