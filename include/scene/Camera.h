#pragma once
#include <DirectXMath.h>

namespace scene {

class Camera
{
public:
    DirectX::XMFLOAT3 position = { 0.0f, 0.0f,  0.0f };
    DirectX::XMFLOAT3 forward  = { 0.0f, 0.0f,  1.0f };
    DirectX::XMFLOAT3 up       = { 0.0f, 1.0f,  0.0f };

    float fovY  = DirectX::XM_PIDIV4; // 45 degrees
    float nearZ = 0.1f;
    float farZ  = 1000.0f;

    DirectX::XMMATRIX viewMatrix() const
    {
        return DirectX::XMMatrixLookToLH(
            DirectX::XMLoadFloat3(&position),
            DirectX::XMLoadFloat3(&forward),
            DirectX::XMLoadFloat3(&up));
    }

    DirectX::XMMATRIX projMatrix(float aspectRatio) const
    {
        return DirectX::XMMatrixPerspectiveFovLH(fovY, aspectRatio, nearZ, farZ);
    }
};

} // namespace scene
