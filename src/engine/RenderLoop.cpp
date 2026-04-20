#include "engine/RenderLoop.h"
#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;

namespace engine {

static void hsvToRgb(float h, float s, float v, float out[3])
{
    int   i = static_cast<int>(h / 60.0f) % 6;
    float f = h / 60.0f - std::floor(h / 60.0f);
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    switch (i)
    {
        case 0: out[0]=v; out[1]=t; out[2]=p; break;
        case 1: out[0]=q; out[1]=v; out[2]=p; break;
        case 2: out[0]=p; out[1]=v; out[2]=t; break;
        case 3: out[0]=p; out[1]=q; out[2]=v; break;
        case 4: out[0]=t; out[1]=p; out[2]=v; break;
        default: out[0]=v; out[1]=p; out[2]=q; break;
    }
}

void RenderLoop::init(HWND hwnd, UINT width, UINT height)
{
    m_device.init(hwnd, width, height);
    updateCamera(); // seed camera at t=0 before first frame
}

void RenderLoop::tick(float dt)
{
    m_follower.update(dt);
    updateCamera();

    m_hue = std::fmod(m_hue + dt * 72.0f, 360.0f);
    float rgb[3];
    hsvToRgb(m_hue, 0.7f, 0.35f, rgb);
    const float clearColor[4] = { rgb[0], rgb[1], rgb[2], 1.0f };

    m_device.beginFrame(clearColor);
    // geometry and lighting passes inserted here from task 5 onward
    m_device.endFrame();
}

void RenderLoop::resize(UINT width, UINT height)
{
    m_device.resize(width, height);
}

void RenderLoop::shutdown()
{
    m_device.shutdown();
}

void RenderLoop::updateCamera()
{
    // Current position on path.
    const world::PathSample curr = m_path.sample(m_follower.t());

    // Lookahead position for aim direction.
    const float lookArc = m_follower.arc() + k_lookaheadArc;
    const world::PathSample look = m_path.sample(m_follower.arcToT(lookArc));

    // Camera sits k_cameraHeightOffset above the path point.
    m_camera.position = {
        curr.position.x,
        curr.position.y + k_cameraHeightOffset,
        curr.position.z
    };

    // Forward vector aims from camera position toward the lookahead position.
    const XMVECTOR eye  = XMLoadFloat3(&m_camera.position);
    const XMVECTOR tgt  = XMLoadFloat3(&look.position);
    const XMVECTOR fwd  = XMVector3Normalize(XMVectorSubtract(tgt, eye));
    XMStoreFloat3(&m_camera.forward, fwd);
}

} // namespace engine
