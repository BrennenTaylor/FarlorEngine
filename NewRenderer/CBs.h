#pragma once

#include <FMath/FMath.h>

#include <DirectXMath.h>

namespace Farlor {
struct cbPerObject {
    DirectX::XMMATRIX WVP;
};


struct cbTransforms {
    DirectX::XMMATRIX World;
    DirectX::XMMATRIX WorldView;
    DirectX::XMMATRIX WorldViewProjection;
};

struct cbMatProperties {
    Vector3 SpecularAlbedo;
    float SpecularPower;
};

struct cbLightProperties {
    Vector3 m_position;
    float _pad1;
    Vector3 m_color;
    float _pad2;
    Vector3 m_direction;
    float _pad3;
    Vector2 m_spotlightAngles;
    Vector2 _pad4;
    Vector4 m_lightRange;
};

struct cbCameraParams {
    Vector3 m_eyePosition;
    float _pad;
};

struct cbTonemappingParams {
    uint32_t m_gridResolutionX;
    uint32_t m_gridResolutionY;
    float _pad1;
    float _pad2;
};
}
