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

#include "../include/platform.h"
#include <Windows.h>

#define RENDERER_WIN32_CLASS_NAME L"RendererClass"
// Copying most of the things from GLFW .. What else to do, me knows nothing else

struct Win32Window
{
    HWND   handle;
    HICON  icon;
    bool   cursorTracked;
    bool   frameAction;
    bool   maximized;

    bool   transparent;

    UINT32 width, height;
    INT32  lastCurPosX, lastCurPosY;

    struct
    {
        BITMAPINFO bmpInfo;
        uint8_t *  colorBuffer;
    } renderBuffer;

    struct
    {
        LARGE_INTEGER frequency;
        LARGE_INTEGER counter;
    } timer;

    WINDOWPLACEMENT wPlacement = {sizeof(WINDOWPLACEMENT)};
};

