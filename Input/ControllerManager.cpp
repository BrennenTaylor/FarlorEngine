#include "ControllerManager.h"

namespace Farlor
{
    ControllerManager::ControllerManager()
        : m_connectionStatus{ false }
        , m_initialized{ false }
        , m_previousState{ 0 }
    {
    }

    ControllerManager::~ControllerManager()
    {
        // Force it off
        SetVibration(0.0f, 0.0f);
    }

    // Returns true if the controller is now connected or the new state is different than the old
    void ControllerManager::Poll()
    {
        XINPUT_STATE state = { 0 };
        // Default to only getting one controller for now
        HRESULT result = XInputGetState(0, &state);
        if (result != ERROR_SUCCESS)
        {
            m_needsUpdate = false;
            m_connectionStatus = false;
            return;
        }

        // If we are newly connected, we always want to inspect
        if (m_connectionStatus == false)
        {
            m_needsUpdate = true;
            m_connectionStatus = true;
            m_previousState = state;
        }
        else
        {
            if (m_previousState.dwPacketNumber != state.dwPacketNumber)
            {
                m_needsUpdate = true;
                m_previousState = state;
            }
            else
            {
                m_needsUpdate = false;
            }
        }
    }

    bool ControllerManager::IsConnected() const
    {
        return m_connectionStatus;
    }

    bool ControllerManager::NeedsUpdate() const
    {
        return m_needsUpdate;
    }

    void ControllerManager::SetVibration(float leftPercent, float rightPercent)
    {
        if (!IsConnected())
        {
            // TODO: Log that controller isnt connected
            return;
        }

        // Message input to a correct range
        leftPercent = max(leftPercent, 0.0f);
        leftPercent = min(leftPercent, 1.0f);
        rightPercent = max(leftPercent, 0.0f);
        rightPercent = min(leftPercent, 1.0f);

        // Set vibration state
        const float maxVibrate = 65535;
        // https://msdn.microsoft.com/en-us/library/windows/desktop/microsoft.directx_sdk.reference.xinput_vibration(v=vs.85).aspx
        XINPUT_VIBRATION vibrationState = { 0 };
        vibrationState.wLeftMotorSpeed = static_cast<uint32_t>(leftPercent * maxVibrate);
        vibrationState.wRightMotorSpeed = static_cast<uint32_t>(rightPercent * maxVibrate);
        auto result = XInputSetState(0, &vibrationState);
        if (result != ERROR_SUCCESS)
        {
            // TODO: Log error
        }
    }

    bool ControllerManager::IsButtonDown(uint32_t controllerButtonIndex) const
    {
        if (!IsConnected())
        {
            // TODO: Log that the state is invalid
            return false;
        }

        switch (controllerButtonIndex)
        {
            case ControllerButtons::DPadUp:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP ? true : false;
            } break;

            case ControllerButtons::DPadDown:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN ? true : false;
            } break;

            case ControllerButtons::DPadLeft:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT ? true : false;
            } break;

            case ControllerButtons::DPadRight:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ? true : false;
            } break;

            case ControllerButtons::Start:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_START ? true : false;
            } break;

            case ControllerButtons::Select:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK ? true : false;
            } break;

            case ControllerButtons::LeftThumb:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB ? true : false;
            } break;

            case ControllerButtons::RightThumb:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB ? true : false;
            } break;

            case ControllerButtons::LeftBumper:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER ? true : false;
            } break;

            case ControllerButtons::RightBumper:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER ? true : false;
            } break;

            case ControllerButtons::A:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_A ? true : false;
            } break;

            case ControllerButtons::B:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_B ? true : false;
            } break;
            
            case ControllerButtons::X:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_X ? true : false;
            } break;

            case ControllerButtons::Y:
            {
                return m_previousState.Gamepad.wButtons & XINPUT_GAMEPAD_Y ? true : false;
            } break;

            default:
            {
                // Log unsuported button case
                return false;
            } break;
        }
    }
}
