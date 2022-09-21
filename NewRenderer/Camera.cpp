#include "Camera.h"

#include "../Input/InputStateManager.h"
#include "FMath/Vector3.h"
#include "FMath/Vector4.h"
#include "SimpleMath.h"
#include <DirectXMath.h>

#undef near
#undef far

namespace Farlor {
void Frustrum::Update(const DirectX::XMFLOAT4X4 &camProjection, const DirectX::XMFLOAT4X4 &camView)
{
    // Create the frustum matrix from the view matrix and updated projection matrix.
    DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&camView);
    DirectX::XMMATRIX projection = DirectX::XMLoadFloat4x4(&camProjection);
    DirectX::XMMATRIX matrix = DirectX::XMMatrixMultiply(view, projection);

    // Calculate near plane of frustum.
    DirectX::XMFLOAT4X4 matrixFloat;
    DirectX::XMStoreFloat4x4(&matrixFloat, matrix);

    // Near plane
    m_planes[0].x = matrixFloat._13;
    m_planes[0].y = matrixFloat._23;
    m_planes[0].z = matrixFloat._33;
    m_planes[0].w = matrixFloat._43;
    m_planes[0].Normalize();

    // Far plane
    m_planes[1].x = matrixFloat._14 - matrixFloat._13;
    m_planes[1].y = matrixFloat._24 - matrixFloat._23;
    m_planes[1].z = matrixFloat._34 - matrixFloat._33;
    m_planes[1].w = matrixFloat._44 - matrixFloat._43;
    m_planes[1].Normalize();

    // Left plane
    m_planes[2].x = matrixFloat._14 + matrixFloat._11;
    m_planes[2].y = matrixFloat._24 + matrixFloat._21;
    m_planes[2].z = matrixFloat._34 + matrixFloat._31;
    m_planes[2].w = matrixFloat._44 + matrixFloat._41;
    m_planes[2].Normalize();

    // Right plane
    m_planes[3].x = matrixFloat._14 - matrixFloat._11;
    m_planes[3].y = matrixFloat._24 - matrixFloat._21;
    m_planes[3].z = matrixFloat._34 - matrixFloat._31;
    m_planes[3].w = matrixFloat._44 - matrixFloat._41;
    m_planes[3].Normalize();

    // Top Plane
    m_planes[4].x = matrixFloat._14 - matrixFloat._12;
    m_planes[4].y = matrixFloat._24 - matrixFloat._22;
    m_planes[4].z = matrixFloat._34 - matrixFloat._32;
    m_planes[4].w = matrixFloat._44 - matrixFloat._42;
    m_planes[4].Normalize();

    // Bottom Plane
    m_planes[5].x = matrixFloat._14 + matrixFloat._12;
    m_planes[5].y = matrixFloat._24 + matrixFloat._22;
    m_planes[5].z = matrixFloat._34 + matrixFloat._32;
    m_planes[5].w = matrixFloat._44 + matrixFloat._42;
    m_planes[5].Normalize();

    return;
}

Camera::Camera()
    : m_width(800)
    , m_height(600)
    , m_near(0.1f)
    , m_far(1000.0f)
    , m_fov(m_minFov)
    , m_heightScalar(1.0f, 1.0f, 1.7f)
    , m_tieCamToPlane(false)
{
    m_camPosition = DirectX::XMFLOAT4(0.0f, 100.0f, 0.0f, 1.0f);
    m_camTarget = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

    m_camRight = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
    m_camForward = DirectX::XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);

    m_moveLeftRight = 0.0f;
    m_moveBackForward = 0.0f;
    m_moveUpDown = 0.0f;

    m_camYaw = 0.0f;
    m_camPitch = 0.0f;

    m_speed = 1.4f;
    m_rotateSpeed = 1.1f;

    UpdateViewport();
    UpdateFrustrum();
}

Camera::Camera(uint32_t width, uint32_t height, float near, float far, float fov)
    : m_width(width)
    , m_height(height)
    , m_near(near)
    , m_far(far)
    , m_fov(m_minFov)
    , m_heightScalar(1.0f, 1.0f, 1.7f)
    , m_tieCamToPlane(false)
{
    // LHS
    m_camRight = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
    m_camForward = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);

    m_moveLeftRight = 0.0f;
    m_moveBackForward = 0.0f;
    m_moveUpDown = 0.0f;

    // Set such that base state is facing +z with +x to the right
    m_camYaw = 0.0f;
    m_camPitch = 0.0f;

    // Meters Per Second
    m_speed = 1.4f;
    m_rotateSpeed = 1.1f;

    UpdateViewport();
    UpdateFrustrum();
}

void Camera::Update(
      const float dt, const InputState &inputState, const PhysicsSystem &physicsSystem)
{
    if (!m_IsMovable) {
        return;
    }

    switch (inputState.m_latestInputType) {
        case InputType::CONTROLLER: {
            DoControllerMove(dt, inputState);
        } break;

        case InputType::KEYBOARD_MOUSE:
        default: {
            DoKeyboardMouseMove(dt, inputState);
        } break;
    };

    m_heightScalar.Update(dt);

    // We disallow any rolling
    DirectX::XMMATRIX rotationMatrix
          = DirectX::XMMatrixRotationRollPitchYaw(m_camPitch, m_camYaw, 0.0f);
    DirectX::XMStoreFloat4x4(&m_camRotationMatrix, rotationMatrix);

    // Aligned with Z axis
    DirectX::XMVECTOR defaultForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    DirectX::XMVECTOR defaultRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    DirectX::XMVECTOR camForward = DirectX::XMVector3TransformCoord(defaultForward, rotationMatrix);
    DirectX::XMVECTOR camRight = DirectX::XMVector3TransformCoord(defaultRight, rotationMatrix);
    DirectX::XMVECTOR camUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    DirectX::XMVECTOR position = DirectX::XMLoadFloat4(&m_camPosition);

    position = DirectX::XMVectorAdd(position, DirectX::XMVectorScale(camRight, m_moveLeftRight));
    position
          = DirectX::XMVectorAdd(position, DirectX::XMVectorScale(camForward, m_moveBackForward));
    position = DirectX::XMVectorAdd(position, DirectX::XMVectorScale(camUp, m_moveUpDown));
    DirectX::XMStoreFloat4(&m_camPosition, position);

    if (m_isCrouched) {
        m_heightScalar.SetTarget(0.7f);
    } else {
        m_heightScalar.SetTarget(1.0f);
    }


    if (m_tieCamToPlane) {
        Farlor::Vector3 intersectionPosition;
        bool intersection = physicsSystem.CastRay(
              Farlor::Vector3(m_camPosition.x, m_camPosition.y + 100.0f, m_camPosition.z * -1.0f),
              Farlor::Vector3(0.0f, -1.0f, 0.0f), 5000.0f, intersectionPosition);
        if (intersection) {
            float height = 1.8f * m_heightScalar.GetCurrent();
            m_camPosition.y = intersectionPosition.y + height;
            position = DirectX::XMLoadFloat4(&m_camPosition);
        }
    }

    DirectX::XMVECTOR target = DirectX::XMVectorAdd(position, camForward);
    DirectX::XMStoreFloat4(&m_camTarget, target);

    m_moveLeftRight = 0.0f;
    m_moveBackForward = 0.0f;
    m_moveUpDown = 0.0f;

    UpdateViewMatrix();
    UpdateFrustrum();
}


void Camera::SetViewport(float width, float height, float nearVal, float farVal, float fov)
{
    m_width = width;
    m_height = height;
    m_near = nearVal;
    m_far = farVal;
    m_fov = fov;
    UpdateViewport();
}

void Camera::UpdateViewport()
{
    m_camViewport = DirectX::SimpleMath::Viewport(0.0f, 0.0f, m_width, m_height, 0.0f, 1.0f);
    DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(
          m_fov, static_cast<float>(m_width) / static_cast<float>(m_height), m_near, m_far);
    DirectX::XMStoreFloat4x4(&m_camProjection, projection);

    std::cout << "New fov: " << m_fov << std::endl;

    UpdateViewProj();
    UpdateFrustrum();
}

void Camera::UpdateFrustrum() { m_frustrum.Update(m_camProjection, m_camView); }

void Camera::UpdateViewProj()
{
    DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&m_camView);
    DirectX::XMMATRIX projection = DirectX::XMLoadFloat4x4(&m_camProjection);
    DirectX::XMMATRIX viewProj = view * projection;
    DirectX::XMVECTOR invViewProjDet = DirectX::XMMatrixDeterminant(viewProj);
    DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(&invViewProjDet, viewProj);
    DirectX::XMMATRIX previousViewProj = DirectX::XMLoadFloat4x4(&m_prevViewProjection);
    DirectX::XMMATRIX reprojectionMatrix = invViewProj * previousViewProj;

    DirectX::XMStoreFloat4x4(&m_camProjection, projection);
    DirectX::XMStoreFloat4x4(&m_camViewProjection, viewProj);
    DirectX::XMStoreFloat4x4(&m_prevViewProjection, invViewProj);
    DirectX::XMStoreFloat4x4(&m_reprojectionMatrix, reprojectionMatrix);
}

void Camera::UpdateViewMatrix()
{
    DirectX::XMVECTOR pos = DirectX::XMLoadFloat4(&m_camPosition);
    DirectX::XMVECTOR target = DirectX::XMLoadFloat4(&m_camTarget);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
    DirectX::XMStoreFloat4x4(&m_camView, view);

    UpdateViewProj();
}

void Camera::DoKeyboardMouseMove(float dt, const InputState &inputState)
{
    if (!m_IsMovable) {
        return;
    }

    if (inputState.m_mouseButtonStates[MouseButtons::Left].endedDown) {
        m_camYaw
              += (float)(inputState.m_currentMousePos.x - inputState.m_previousMousePos.x) * 0.01f;
        m_camPitch
              += (float)(inputState.m_currentMousePos.y - inputState.m_previousMousePos.y) * 0.01f;
    }

    if (inputState.m_keyboardButtonStates[KeyboardButtons::A].endedDown) {
        m_moveLeftRight -= dt * (m_isSprinting ? m_speed * 20.0f : m_speed);
    }
    if (inputState.m_keyboardButtonStates[KeyboardButtons::D].endedDown) {
        m_moveLeftRight += dt * (m_isSprinting ? m_speed * 20.0f : m_speed);
    }
    if (inputState.m_keyboardButtonStates[KeyboardButtons::W].endedDown) {
        m_moveBackForward += dt * (m_isSprinting ? m_speed * 20.0f : m_speed);
    }
    if (inputState.m_keyboardButtonStates[KeyboardButtons::S].endedDown) {
        m_moveBackForward -= dt * (m_isSprinting ? m_speed * 20.0f : m_speed);
    }
    if (inputState.m_keyboardButtonStates[KeyboardButtons::Q].endedDown) {
        m_moveUpDown += dt * (m_isSprinting ? m_speed * 20.0f : m_speed);
    }
    if (inputState.m_keyboardButtonStates[KeyboardButtons::Ctrl].endedDown) {
        m_moveUpDown -= dt * (m_isSprinting ? m_speed * 20.0f : m_speed);
    }
    if (inputState.m_keyboardButtonStates[KeyboardButtons::Shift].endedDown) {
        m_isSprinting = true;
    } else {
        m_isSprinting = false;
    }
    if (inputState.m_keyboardButtonStates[KeyboardButtons::Ctrl].endedDown) {
        m_isCrouched = true;
    } else {
        m_isCrouched = false;
    }
    if (inputState.m_keyboardButtonStates[KeyboardButtons::L].numHalfSteps == 1
          && inputState.m_keyboardButtonStates[KeyboardButtons::L].endedDown) {
        m_tieCamToPlane = !m_tieCamToPlane;
    }
}

void Camera::DoControllerMove(float dt, const InputState &inputState)
{
    if (!m_IsMovable) {
        return;
    }

    const float lx = inputState.m_controllerState.m_leftJoystickXMovingAverage.GetCurrentAvereage();
    const float ly = inputState.m_controllerState.m_leftJoystickYMovingAverage.GetCurrentAvereage();
    const float rx
          = inputState.m_controllerState.m_rightJoystickXMovingAverage.GetCurrentAvereage();
    const float ry
          = inputState.m_controllerState.m_rightJoystickYMovingAverage.GetCurrentAvereage();

    const float lt = inputState.m_controllerState.m_leftTrigger.GetCurrentAvereage();
    const float rt = inputState.m_controllerState.m_RightTrigger.GetCurrentAvereage();

    const float controllerSpeedScale = 0.00005f;

    m_moveLeftRight = dt * lx * controllerSpeedScale * (m_isSprinting ? m_speed * 20.0f : m_speed);
    m_moveBackForward
          = dt * ly * controllerSpeedScale * (m_isSprinting ? m_speed * 20.0f : m_speed);
    m_moveUpDown += dt * lt * (m_isSprinting ? m_speed * 20.0f : m_speed);
    m_moveUpDown -= dt * rt * (m_isSprinting ? m_speed * 20.0f : m_speed);

    m_camYaw += dt * m_rotateSpeed * rx * controllerSpeedScale;
    m_camPitch += dt * m_rotateSpeed * ry * -1.0f * controllerSpeedScale;
}
}
