#pragma once
#include "win32_platform.h"

#include <sstream>

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static Platform         win32Platform;
static Win32Window      win32;

void                    SwapBuffers(Platform *platform)
{
    HDC hdc = GetDC(win32.handle);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR CmdLine, int nCmdShow)
{

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSEX wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = RENDERER_WIN32_CLASS_NAME;

    if (!RegisterClassEx(&wc))
        return -1;

#if _DEBUG
    AllocConsole();
#endif
    // We ain't handling anything input related for now

    win32.handle = CreateWindowEx(WS_EX_APPWINDOW, RENDERER_WIN32_CLASS_NAME, L"Rasterize and Raytracer",
                                  WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
                                  NULL, GetModuleHandle(NULL), nullptr);
    ShowWindow(win32.handle, SW_SHOWNA);

    // Platform initialization
    QueryPerformanceCounter(&win32.timer.counter);
    QueryPerformanceFrequency(&win32.timer.frequency);

    int     frames_count = 0;
    float32 time_elapsed = 0.0f;

    MSG     msg{};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Run other things here
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        win32Platform.deltaTime =
            (counter.QuadPart - win32.timer.counter.QuadPart) * 1.0f / win32.timer.frequency.QuadPart;
        win32.timer.counter = counter;

        if (time_elapsed >= 1.0f)
        {
            std::wstringstream str{};
            str << L"     FPS : " << frames_count << L" Time elapsed : " << 1.0f / frames_count;
            frames_count = 0; 
            time_elapsed -= 1.0f;
            SetWindowText(win32.handle, str.str().c_str());
        }
        time_elapsed = time_elapsed + win32Platform.deltaTime;
        frames_count = frames_count + 1; 
    }

    return 0;
}

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        RECT rect;
        GetClientRect(hwnd, &rect);
        SetWindowPos(hwnd, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_FRAMECHANGED);
        break;
    }

    case WM_CLOSE:
        PostQuitMessage(0);
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC         hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        break;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}