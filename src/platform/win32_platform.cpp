#pragma once
#include "win32_platform.h"
#include <windowsx.h>

#include <sstream>

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static Platform         win32Platform; // --> Exposed to other translation units
static Win32Window      win32;         // --> Used internally for win32 application only

bool                    isKeyPressed(Keys key)
{
    return win32Platform.Keyboard.keymaps[(int32_t)key];
}

static void ToggleFullScreen(HWND hwnd)
{
    DWORD dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
    if (dwStyle & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO mInfo = {sizeof(MONITORINFO)};
        if (GetWindowPlacement(hwnd, &win32.wPlacement) &&
            GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mInfo))
        {
            SetWindowLongPtr(hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
            auto width  = mInfo.rcMonitor.right - mInfo.rcMonitor.left;
            auto height = mInfo.rcMonitor.bottom - mInfo.rcMonitor.top;
            SetWindowPos(hwnd, HWND_TOP, mInfo.rcMonitor.left, mInfo.rcMonitor.top, width, height,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        // Restore the window to its previous position
        SetWindowLongPtr(hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd, &win32.wPlacement);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

static void MapKeyboardKeys()
{
    using enum Keys;
    for (int i = 0; i < 512; ++i)
        win32Platform.Keyboard.keycodes[i] = 127;

    win32Platform.Keyboard.keycodes[(int16_t)'A'] = (int32_t)A;
    win32Platform.Keyboard.keycodes[(int16_t)'B'] = (int32_t)B;
    win32Platform.Keyboard.keycodes[(int16_t)'C'] = (int32_t)C;
    win32Platform.Keyboard.keycodes[(int16_t)'D'] = (int32_t)D;
    win32Platform.Keyboard.keycodes[(int16_t)'E'] = (int32_t)E;
    win32Platform.Keyboard.keycodes[(int16_t)'F'] = (int32_t)F;
    win32Platform.Keyboard.keycodes[(int16_t)'G'] = (int32_t)G;
    win32Platform.Keyboard.keycodes[(int16_t)'H'] = (int32_t)H;
    win32Platform.Keyboard.keycodes[(int16_t)'I'] = (int32_t)I;
    win32Platform.Keyboard.keycodes[(int16_t)'J'] = (int32_t)J;
    win32Platform.Keyboard.keycodes[(int16_t)'K'] = (int32_t)K;
    win32Platform.Keyboard.keycodes[(int16_t)'L'] = (int32_t)L;
    win32Platform.Keyboard.keycodes[(int16_t)'M'] = (int32_t)M;
    win32Platform.Keyboard.keycodes[(int16_t)'N'] = (int32_t)N;
    win32Platform.Keyboard.keycodes[(int16_t)'O'] = (int32_t)O;
    win32Platform.Keyboard.keycodes[(int16_t)'P'] = (int32_t)P;
    win32Platform.Keyboard.keycodes[(int16_t)'Q'] = (int32_t)Q;
    win32Platform.Keyboard.keycodes[(int16_t)'R'] = (int32_t)R;
    win32Platform.Keyboard.keycodes[(int16_t)'S'] = (int32_t)S;
    win32Platform.Keyboard.keycodes[(int16_t)'T'] = (int32_t)T;
    win32Platform.Keyboard.keycodes[(int16_t)'U'] = (int32_t)U;
    win32Platform.Keyboard.keycodes[(int16_t)'V'] = (int32_t)V;
    win32Platform.Keyboard.keycodes[(int16_t)'W'] = (int32_t)W;
    win32Platform.Keyboard.keycodes[(int16_t)'X'] = (int32_t)X;
    win32Platform.Keyboard.keycodes[(int16_t)'Y'] = (int32_t)Y;
    win32Platform.Keyboard.keycodes[(int16_t)'Z'] = (int32_t)Z;
}

static void SetWindowOpacity(HWND hwnd, float opacity)
{
    if (opacity < 1.0f)
    {
        const BYTE alpha = (BYTE)(255 * opacity);
        DWORD      style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, style | WS_EX_LAYERED);
        SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
    }
}

static void SetOpacity(float opacity)
{
    SetWindowOpacity(win32.handle, opacity);
}

void SwapBuffers(void)
{
    HDC hdc = GetDC(win32.handle);
    StretchDIBits(hdc, 0, 0, win32Platform.width, win32Platform.height, 0, 0, win32Platform.width, win32Platform.height,
                  win32.renderBuffer.colorBuffer, &win32.renderBuffer.bmpInfo, DIB_RGB_COLORS, SRCCOPY);
}

Platform GetCurrentPlatform(void)
{
    return win32Platform;
}

void CreateWritableBitmap(uint32_t width, uint32_t height)
{
    // Parameters are ignored .. lol

    BITMAPINFO       bmpInfo{};
    BITMAPINFOHEADER bmpHeader{};

    bmpHeader.biSize           = sizeof(bmpHeader);
    bmpHeader.biWidth          = width;
    bmpHeader.biHeight         = -(int32_t)height; // For top to bottom bmp -> Origin top left corner
    bmpHeader.biPlanes         = 1;
    bmpHeader.biBitCount       = 32; // For padding
    bmpHeader.biCompression    = BI_RGB;
    bmpHeader.biSizeImage      = 0;
    bmpInfo.bmiHeader          = bmpHeader;

    win32.renderBuffer.bmpInfo = bmpInfo;
}

void ResizeWritableBitmap(uint32_t width, uint32_t height)
{
    win32.renderBuffer.bmpInfo.bmiHeader.biWidth  = width;
    win32.renderBuffer.bmpInfo.bmiHeader.biHeight = -(int32_t)height;
    if (win32.renderBuffer.colorBuffer)
        VirtualFree(win32.renderBuffer.colorBuffer, 0, MEM_RELEASE);

    uint8_t bytesPerPixel = win32.renderBuffer.bmpInfo.bmiHeader.biBitCount / 8;

    win32.renderBuffer.colorBuffer =
        (uint8_t *)VirtualAlloc(nullptr, width * height * bytesPerPixel, MEM_COMMIT, PAGE_READWRITE);

    // platform specific
    win32Platform.width                  = width;
    win32Platform.height                 = height;
    win32Platform.colorBuffer.noChannels = win32.renderBuffer.bmpInfo.bmiHeader.biBitCount / 8;
    win32Platform.colorBuffer.width      = width;
    win32Platform.colorBuffer.height     = height;
    win32Platform.colorBuffer.buffer     = win32.renderBuffer.colorBuffer;

    if (win32Platform.zBuffer.buffer)
        VirtualFree(win32Platform.zBuffer.buffer, 0, MEM_RELEASE);

    uint32_t zMemorySize = width * height * sizeof(float);
    win32Platform.zBuffer.buffer =
        (float *)VirtualAlloc(nullptr, width * height * sizeof(float), MEM_COMMIT, PAGE_READWRITE);
    win32Platform.zBuffer.width  = width;
    win32Platform.zBuffer.height = height;

    assert(win32.renderBuffer.colorBuffer != nullptr);
    assert(win32Platform.zBuffer.buffer);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR CmdLine, int nCmdShow)
{

    // SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
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
    FILE *fp;
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONOUT$", "w", stdout);
#endif
    // We ain't handling anything input related for now

    win32.handle =
        CreateWindowEx(WS_EX_APPWINDOW, RENDERER_WIN32_CLASS_NAME, L"Rasterize and Raytracer", WS_OVERLAPPEDWINDOW,
                       CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, GetModuleHandle(NULL), nullptr);
    ShowWindow(win32.handle, SW_SHOWNA);

    // Platform initialization
    QueryPerformanceCounter(&win32.timer.counter);
    QueryPerformanceFrequency(&win32.timer.frequency);

    win32Platform.SwapBuffer  = SwapBuffers;
    win32Platform.SetOpacity  = SetOpacity;
    win32Platform.bKeyPressed = isKeyPressed;
    MapKeyboardKeys();

    // Lets register a new raw input device for unlimited mouse movement
    RAWINPUTDEVICE rawMouse{};
    rawMouse.usUsagePage = 0x01;
    rawMouse.usUsage     = 0x02;
    rawMouse.hwndTarget  = win32.handle;

    if (!RegisterRawInputDevices(&rawMouse, 1, sizeof(rawMouse)))
    {
        MessageBox(win32.handle, L"Failed to register raw input", L"Nothing much", MB_OK);
    }

    int     frames_count = 0;
    float32 time_elapsed = 0.0f;

    MSG     msg{};
    while (msg.message != WM_QUIT)
    {
        ZeroMemory(win32Platform.Keyboard.keymaps, sizeof(win32Platform.Keyboard.keymaps));
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

        RendererMainLoop(&win32Platform);

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
    case WM_NCCREATE:
    {
        CreateWritableBitmap(win32Platform.height, win32Platform.height);
        SetCursorPos(300, 300);
        win32Platform.Mouse.xpos = 300;
        win32Platform.Mouse.ypos = 300;
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    case WM_CREATE:
    {
        RECT rect;
        GetClientRect(hwnd, &rect);
        SetWindowPos(hwnd, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_FRAMECHANGED);

        OutputDebugString(L"WM_CREATE\n");
        break;
    }

    case WM_CLOSE:
        PostQuitMessage(0);
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC         hdc = BeginPaint(hwnd, &ps);
        // FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_SIZE:
    {
        // TODO : Handle when (width | height -> 0) or (Minimized event is sent)
        win32Platform.bFirst = true;
        RECT rect;
        GetClientRect(hwnd, &rect);
        win32Platform.width  = rect.right - rect.left;
        win32Platform.height = rect.bottom - rect.top;
        ResizeWritableBitmap(win32Platform.width, win32Platform.height);
        OutputDebugString(L"WM_SIZE\n");
        break;
    }
    case WM_MOUSEMOVE:
    {
       /* win32Platform.Mouse.xpos = GET_X_LPARAM(lParam);
        win32Platform.Mouse.ypos = GET_Y_LPARAM(lParam);*/
        break;
    }
    case WM_MOUSEWHEEL:
    {
        win32Platform.Mouse.bMouseScrolled = true;
        win32Platform.bFirst               = true;
        win32Platform.Mouse.value          = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        break;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
        const int action   = HIWORD(lParam) & KF_UP; // <-- If action is 1, key is released else pressed
        int       scancode = (HIWORD(lParam) & (KF_EXTENDED | 0xFF));
        if (wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN) && action)
        {
            ToggleFullScreen(hwnd);
        }
        win32Platform.Keyboard.keymaps[win32Platform.Keyboard.keycodes[wParam]] = 1;
        break;
    }

    case WM_INPUT:
    {
        // For raw input
        UINT      size = 0;
        HRAWINPUT ri   = (HRAWINPUT)lParam;
        RAWINPUT *data = nullptr;
        int32_t   dx, dy;
        GetRawInputData(ri, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
        auto lpb = new BYTE[size];
        if (!lpb)
            break;
        if (GetRawInputData(ri, RID_INPUT, lpb, &size, sizeof(RAWINPUTHEADER)) != size)
            OutputDebugString(L"GetRawInputData doesn't return correct size");

        data = (RAWINPUT *)lpb;
        if (data->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
        {
            dx = data->data.mouse.lLastX - win32Platform.Mouse.xpos;
            dy = data->data.mouse.lLastY - win32Platform.Mouse.ypos;
        }
        else
        {
            dx = data->data.mouse.lLastX;
            dy = data->data.mouse.lLastY;
        }
        win32Platform.Mouse.xpos += dx; 
        win32Platform.Mouse.ypos += dy; 
        delete[] lpb;
        break;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}