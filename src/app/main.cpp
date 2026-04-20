#include <Windows.h>
#include "util/log.h"
#include "util/hr.h"
#include "engine/RenderLoop.h"

static constexpr UINT k_width  = 1280;
static constexpr UINT k_height = 720;

// File-static pointer so WndProc can forward WM_SIZE without global state leaking further.
static engine::RenderLoop* g_renderLoop = nullptr;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        if (g_renderLoop && wParam != SIZE_MINIMIZED)
            g_renderLoop->resize(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"TemeraldWnd";
    RegisterClassExW(&wc);

    RECT rect{ 0, 0, static_cast<LONG>(k_width), static_cast<LONG>(k_height) };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowExW(
        0,
        L"TemeraldWnd",
        L"temerald",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, hInstance, nullptr
    );

    LOG_INFO("boot — temerald initialised, window %ux%u", k_width, k_height);

    engine::RenderLoop renderLoop;
    g_renderLoop = &renderLoop;
    renderLoop.init(hwnd, k_width, k_height);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    LARGE_INTEGER freq, prev;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    MSG msg{};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        else
        {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            const float dt = static_cast<float>(now.QuadPart - prev.QuadPart)
                           / static_cast<float>(freq.QuadPart);
            prev = now;

            renderLoop.tick(dt);
        }
    }

    renderLoop.shutdown();
    g_renderLoop = nullptr;

    return static_cast<int>(msg.wParam);
}
