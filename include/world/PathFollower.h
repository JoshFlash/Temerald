#pragma once
#include "world/GuidedPath.h"
#include <vector>

namespace world {

class PathFollower
{
public:
    explicit PathFollower(const GuidedPath& path, float speedUnitsPerSec = 8.0f);

    // Advances arc-length position by speed*dt; polls keyboard for W/S/Space.
    void update(float dt);

    float t()    const { return m_t; }
    float arc()  const { return m_arc; }
    float speed() const { return m_speed; }
    bool  paused() const { return m_paused; }

    // Convert an arc-length value to the corresponding path parameter t.
    // Safe to call with arc values beyond totalArcLength (clamped).
    float arcToT(float arc) const;

private:
    void buildLut(const GuidedPath& path);

    struct LutEntry { float t; float arcLength; };
    std::vector<LutEntry> m_lut;

    float m_totalArcLength = 0.0f;
    float m_t              = 0.0f;
    float m_arc            = 0.0f;
    float m_speed;
    bool  m_paused         = false;
    bool  m_spacePrev      = false; // edge-detect for Space toggle
};

} // namespace world
