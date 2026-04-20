#pragma once
#include "render/Device.h"

namespace engine {

class RenderLoop
{
public:
    void init(HWND hwnd, UINT width, UINT height);
    void tick(float dt);
    void resize(UINT width, UINT height);
    void shutdown();

private:
    render::Device m_device;
    float          m_hue = 0.0f;
};

} // namespace engine
