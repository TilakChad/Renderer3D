#pragma once
#include <cassert>
#include <cstdint>

using float32 = float;
using float64 = double;

struct Platform;

enum class Keys
{
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    NUM1,
    NUM2,
    NUM3,
    NUM4,
    NUM5,
    NUM6,
    NUM7,
    NUM8,
    NUM9,
    NUM0,
    ENTER,
    ESCAPE,
    SPACE,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    BACKSPACE
};

typedef void (*SwapBufferFn)(void);
typedef void (*OpacityFn)(float);
typedef bool (*KeyPressedFn)(Keys key);

struct Platform
{
    // What should a platform have?
    uint32_t height;
    uint32_t width;
    float32  deltaTime;

    struct
    {
        uint8_t *buffer;
        uint32_t width;
        uint32_t height;
        uint8_t  noChannels;
    } colorBuffer;

    struct
    {
        float   *buffer;
        uint32_t width    = 0;
        uint32_t height   = 0;
        uint32_t channels = 1;
    } shadowMap;

    struct
    {
        // No stencil buffer, only depth buffer for now.. with single precision floating point depth
        float   *buffer = nullptr;
        uint32_t width;
        uint32_t height;
    } zBuffer;

    SwapBufferFn SwapBuffer;
    OpacityFn    SetOpacity;
    KeyPressedFn bKeyPressed;
    bool         bFirst       = true;
    bool         bSizeChanged = false;
    struct
    {
        bool    bMouseScrolled = false;
        int32_t value          = 0;
        int32_t xpos           = 0;
        int32_t ypos           = 0;
    } Mouse;

    struct Keyboard
    {
        // Each key from windows need to be mapped with key from our enum
        int16_t keycodes[512];
        int16_t keymaps[128];
    } Keyboard;
};

void     RendererMainLoop(Platform *platform);
Platform GetCurrentPlatform(void);
