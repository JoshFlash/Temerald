#include "world/GuidedPath.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace world {

// ── Construction ──────────────────────────────────────────────────────────────

GuidedPath::GuidedPath()
{
    // ~12 control points along +Z, 20 m per segment, gentle ±3 m X jitter.
    // Y = 1.0 m keeps the path at roughly eye level above ground.
    m_points = {
        {  0.0f, 1.0f,   0.0f },
        {  1.5f, 1.0f,  20.0f },
        { -2.0f, 1.0f,  40.0f },
        {  2.5f, 1.0f,  60.0f },
        { -1.0f, 1.0f,  80.0f },
        {  1.0f, 1.0f, 100.0f },
        { -2.5f, 1.0f, 120.0f },
        {  2.0f, 1.0f, 140.0f },
        { -1.5f, 1.0f, 160.0f },
        {  1.0f, 1.0f, 180.0f },
        { -2.0f, 1.0f, 200.0f },
        {  0.5f, 1.0f, 220.0f },
    };
}

// ── Public ────────────────────────────────────────────────────────────────────

PathSample GuidedPath::sample(float t) const
{
    const int n = static_cast<int>(m_points.size());
    t = std::clamp(t, 0.0f, static_cast<float>(n - 1));

    // Segment index and local parameter.
    int   seg  = static_cast<int>(t);
    float frac = t - static_cast<float>(seg);

    // Clamp segment so we always have a valid i+1.
    if (seg >= n - 1) { seg = n - 2; frac = 1.0f; }

    // Clamped neighbouring control points for boundary segments.
    const XMFLOAT3& p1 = m_points[seg];
    const XMFLOAT3& p2 = m_points[seg + 1];
    const XMFLOAT3& p0 = m_points[std::max(seg - 1, 0)];
    const XMFLOAT3& p3 = m_points[std::min(seg + 2, n - 1)];

    PathSample result;
    result.position = evalPosition(p0, p1, p2, p3, frac);

    const XMFLOAT3 rawTan = evalTangent(p0, p1, p2, p3, frac);
    XMStoreFloat3(&result.tangent, XMVector3Normalize(XMLoadFloat3(&rawTan)));

    return result;
}

// ── Private ───────────────────────────────────────────────────────────────────

// Uniform Catmull-Rom position.
// P(t) = 0.5 * [ 2P1 + (-P0+P2)t + (2P0-5P1+4P2-P3)t² + (-P0+3P1-3P2+P3)t³ ]
XMFLOAT3 GuidedPath::evalPosition(
    const XMFLOAT3& p0, const XMFLOAT3& p1,
    const XMFLOAT3& p2, const XMFLOAT3& p3,
    float t)
{
    const XMVECTOR vp0 = XMLoadFloat3(&p0);
    const XMVECTOR vp1 = XMLoadFloat3(&p1);
    const XMVECTOR vp2 = XMLoadFloat3(&p2);
    const XMVECTOR vp3 = XMLoadFloat3(&p3);

    const float t2 = t * t;
    const float t3 = t2 * t;

    const XMVECTOR result = XMVectorScale(
          XMVectorAdd(
              XMVectorScale(vp1, 2.0f),
          XMVectorAdd(
              XMVectorScale(XMVectorAdd(XMVectorNegate(vp0), vp2), t),
          XMVectorAdd(
              XMVectorScale(XMVectorAdd(XMVectorAdd(XMVectorScale(vp0, 2.0f),
                                                    XMVectorScale(vp1, -5.0f)),
                                        XMVectorAdd(XMVectorScale(vp2, 4.0f),
                                                    XMVectorNegate(vp3))), t2),
              XMVectorScale(XMVectorAdd(XMVectorAdd(XMVectorNegate(vp0),
                                                    XMVectorScale(vp1, 3.0f)),
                                        XMVectorAdd(XMVectorScale(vp2, -3.0f),
                                                    vp3)), t3)))),
        0.5f);

    XMFLOAT3 out;
    XMStoreFloat3(&out, result);
    return out;
}

// Derivative of the Catmull-Rom polynomial.
// P'(t) = 0.5 * [ (-P0+P2) + 2(2P0-5P1+4P2-P3)t + 3(-P0+3P1-3P2+P3)t² ]
XMFLOAT3 GuidedPath::evalTangent(
    const XMFLOAT3& p0, const XMFLOAT3& p1,
    const XMFLOAT3& p2, const XMFLOAT3& p3,
    float t)
{
    const XMVECTOR vp0 = XMLoadFloat3(&p0);
    const XMVECTOR vp1 = XMLoadFloat3(&p1);
    const XMVECTOR vp2 = XMLoadFloat3(&p2);
    const XMVECTOR vp3 = XMLoadFloat3(&p3);

    const float t2 = t * t;

    const XMVECTOR coeff1 = XMVectorAdd(XMVectorNegate(vp0), vp2);
    const XMVECTOR coeff2 = XMVectorAdd(XMVectorAdd(XMVectorScale(vp0, 2.0f),
                                                     XMVectorScale(vp1, -5.0f)),
                                         XMVectorAdd(XMVectorScale(vp2, 4.0f),
                                                     XMVectorNegate(vp3)));
    const XMVECTOR coeff3 = XMVectorAdd(XMVectorAdd(XMVectorNegate(vp0),
                                                     XMVectorScale(vp1, 3.0f)),
                                         XMVectorAdd(XMVectorScale(vp2, -3.0f),
                                                     vp3));

    const XMVECTOR result = XMVectorScale(
        XMVectorAdd(coeff1,
        XMVectorAdd(XMVectorScale(coeff2, 2.0f * t),
                    XMVectorScale(coeff3, 3.0f * t2))),
        0.5f);

    XMFLOAT3 out;
    XMStoreFloat3(&out, result);
    return out;
}

} // namespace world
