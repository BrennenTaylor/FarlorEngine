#pragma once

#include "Effects.h"
#undef min
#undef max
#include "../Physics/PhysicsSystem.h"

#define WIN_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3d11.h>
#include <DirectXMath.h>
#include <SimpleMath.h>

#include <FMath/FMath.h>

namespace Farlor {
struct InputState;

class InterpolatedValue {
   public:
    InterpolatedValue(const float startValue, const float endValue, const float speed)
        : m_current(startValue)
        , m_target(endValue)
        , m_speed(speed)
    {
    }

    float GetCurrent() const { return m_current; }

    void SetTarget(const float target) { m_target = target; }
    void SetSpeed(const float speed) { m_speed = speed; }

    void Update(float dt)
    {
        if (abs(m_target - m_current) < 0.001f) {
            return;
        }

        float remainingDistance = m_target - m_current;
        float step = m_speed * dt * (remainingDistance > 0.0f ? 1.0f : -1.0f);
        if (abs(remainingDistance) < abs(step)) {
            step = remainingDistance;
        }
        m_current += step;
    }

   private:
    float m_current = 0.0f;
    float m_target = 0.0f;
    float m_speed = 1.0f;

    void Update();
};

class Frustrum {
   public:
    void Update(const DirectX::XMFLOAT4X4 &projection, const DirectX::XMFLOAT4X4 &viewMatrix);

    bool CheckPoint(const Farlor::Vector3 &point);

   private:
    std::array<DirectX::SimpleMath::Plane, 6> m_planes;
};

class Camera {
   public:
    Camera();
    Camera(uint32_t width, uint32_t height, float near, float far, float fov);
    void Update(const float dt, const InputState &inputState, const PhysicsSystem &physicsSystem);

    DirectX::XMFLOAT4 GetCamPos() const { return m_camPosition; }
    DirectX::XMFLOAT4X4 GetCamView() const { return m_camView; }
    DirectX::XMFLOAT4X4 GetCamProjection() const { return m_camProjection; }
    DirectX::XMFLOAT4X4 GetCamViewProjection() const { return m_camViewProjection; }

    void SetViewport(float width, float height, float nearVal, float farVal, float fov);
    void UpdateViewport();
    DirectX::SimpleMath::Viewport GetViewport() const { return m_camViewport; }

   private:
    void UpdateFrustrum();
    void UpdateViewMatrix();
    void UpdateViewProj();

    void DoKeyboardMouseMove(const float dt, const InputState &inputState);
    void DoControllerMove(const float dt, const InputState &inputState);

   private:
    DirectX::XMFLOAT4X4 m_camView;
    DirectX::XMFLOAT4X4 m_camProjection;
    DirectX::XMFLOAT4X4 m_camViewProjection;
    DirectX::XMFLOAT4X4 m_prevViewProjection;
    DirectX::XMFLOAT4X4 m_reprojectionMatrix;

    DirectX::XMFLOAT4X4 m_camRotationMatrix;

    DirectX::SimpleMath::Viewport m_camViewport;
    Frustrum m_frustrum;

    DirectX::XMFLOAT4 m_camForward = { 0.0f, 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT4 m_camRight = { 0.0f, 0.0f, 0.0f, 0.0f };

    float m_moveLeftRight = 0.0f;
    float m_moveBackForward = 0.0f;
    float m_moveUpDown = 0.0f;

    float m_camYaw = 0.0f;
    float m_camPitch = 0.0f;

    DirectX::XMFLOAT4 m_camPosition = { 0.0f, 0.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT4 m_camTarget = { 0.0f, 0.0f, 0.0f, 1.0f };

    float m_width = 1.0f;
    float m_height = 1.0f;
    float m_near = 1.0f;
    float m_far = 1.0f;

    // In Radians
    float m_minFov = DirectX::XM_PIDIV2 * 0.5f;
    float m_maxFov = DirectX::XM_PI * 0.99f;
    float m_fov = DirectX::XM_PIDIV2 * 0.5f;

    float m_speed = 0.0f;
    float m_rotateSpeed = 0.0f;

    bool m_isSprinting = false;
    bool m_isCrouched = false;
    bool m_tieCamToPlane = false;

    InterpolatedValue m_heightScalar;

    bool m_IsMovable = true;
};
}
