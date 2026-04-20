#include "world/PathFollower.h"
#include <Windows.h>
#include <DirectXMath.h>
#include <algorithm>

using namespace DirectX;

namespace world {

static constexpr int   k_lutSamplesPerUnit = 100; // samples per unit of t
static constexpr float k_maxSpeed          = 50.0f;

PathFollower::PathFollower(const GuidedPath& path, float speedUnitsPerSec)
    : m_speed(speedUnitsPerSec)
{
    buildLut(path);
}

void PathFollower::update(float dt)
{
    // ── Keyboard overrides (W/S nudge speed, Space toggles pause) ──────────
    const bool wDown     = (GetAsyncKeyState('W')      & 0x8000) != 0;
    const bool sDown     = (GetAsyncKeyState('S')      & 0x8000) != 0;
    const bool spaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    if (wDown) m_speed = std::min(m_speed + 5.0f * dt, k_maxSpeed);
    if (sDown) m_speed = std::max(m_speed - 5.0f * dt, 0.0f);

    if (spaceDown && !m_spacePrev) m_paused = !m_paused;
    m_spacePrev = spaceDown;

    // ── Advance arc-length position ────────────────────────────────────────
    if (!m_paused)
    {
        m_arc = std::min(m_arc + m_speed * dt, m_totalArcLength);
        m_t   = arcToT(m_arc);
    }
}

float PathFollower::arcToT(float arc) const
{
    arc = std::clamp(arc, 0.0f, m_totalArcLength);

    // Binary search for the largest entry whose arcLength <= arc.
    int lo = 0;
    int hi = static_cast<int>(m_lut.size()) - 1;
    while (lo < hi - 1)
    {
        const int mid = (lo + hi) / 2;
        if (m_lut[mid].arcLength <= arc) lo = mid;
        else                             hi = mid;
    }

    const LutEntry& a = m_lut[lo];
    const LutEntry& b = m_lut[hi];
    if (b.arcLength <= a.arcLength) return a.t;

    const float blend = (arc - a.arcLength) / (b.arcLength - a.arcLength);
    return a.t + blend * (b.t - a.t);
}

void PathFollower::buildLut(const GuidedPath& path)
{
    const int totalSamples = static_cast<int>(path.maxT() * k_lutSamplesPerUnit) + 1;
    m_lut.reserve(totalSamples);

    float      arc  = 0.0f;
    XMFLOAT3   prev = path.sample(0.0f).position;
    m_lut.push_back({ 0.0f, 0.0f });

    for (int i = 1; i < totalSamples; ++i)
    {
        const float t    = static_cast<float>(i) / static_cast<float>(k_lutSamplesPerUnit);
        const XMFLOAT3 curr = path.sample(t).position;

        const XMVECTOR d = XMVectorSubtract(XMLoadFloat3(&curr), XMLoadFloat3(&prev));
        arc += XMVectorGetX(XMVector3Length(d));

        m_lut.push_back({ t, arc });
        prev = curr;
    }

    m_totalArcLength = arc;
}

} // namespace world
