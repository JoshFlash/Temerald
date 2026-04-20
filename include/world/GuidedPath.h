#pragma once
#include <DirectXMath.h>
#include <vector>

namespace world {

struct PathSample
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 tangent; // unit length
};

class GuidedPath
{
public:
    GuidedPath(); // initialises default ~12 hand-coded control points

    // t in [0, numSegments]; clamped at endpoints.
    // numSegments == controlPointCount - 1.
    PathSample sample(float t) const;

    float maxT() const { return static_cast<float>(m_points.size() - 1); }

    const std::vector<DirectX::XMFLOAT3>& controlPoints() const { return m_points; }

private:
    static DirectX::XMFLOAT3 evalPosition(
        const DirectX::XMFLOAT3& p0, const DirectX::XMFLOAT3& p1,
        const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT3& p3,
        float t);

    static DirectX::XMFLOAT3 evalTangent(
        const DirectX::XMFLOAT3& p0, const DirectX::XMFLOAT3& p1,
        const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT3& p3,
        float t);

    std::vector<DirectX::XMFLOAT3> m_points;
};

} // namespace world
