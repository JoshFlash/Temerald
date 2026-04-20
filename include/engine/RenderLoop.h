#pragma once
#include "render/Device.h"
#include "scene/Camera.h"
#include "world/GuidedPath.h"
#include "world/PathFollower.h"

namespace engine {

class RenderLoop
{
public:
    void init(HWND hwnd, UINT width, UINT height);
    void tick(float dt);
    void resize(UINT width, UINT height);
    void shutdown();

private:
    void updateCamera();

    render::Device       m_device;
    world::GuidedPath    m_path;
    world::PathFollower  m_follower{ m_path };
    scene::Camera        m_camera;

    float m_hue = 0.0f;

    static constexpr float k_cameraHeightOffset = 0.6f;  // metres above path
    static constexpr float k_lookaheadArc       = 5.0f;  // metres ahead for aim
};

} // namespace engine
