#pragma once

#include "ControllerButton.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <XInput.h>

namespace Farlor
{
    class ControllerManager
    {
    public:
        ControllerManager();
        ~ControllerManager();

        // Polls every controller state and checks if any have connected or disconnected
        void Poll();

        // Return true if the specified controller is connected
        // If it is not connected, the polled input is not valid
        bool IsConnected() const;
        bool NeedsUpdate() const;
        bool IsButtonDown(uint32_t controllerButtonIndex) const;

        // Set Vibration States
        void SetVibration(float leftPercent, float rightPercent);

        const XINPUT_STATE& AccessRawXInputState() const {
            return m_previousState;
        }

    private:
        uint32_t m_needsUpdate = false;

        // Tracks which controllers are connected
        uint32_t m_connectionStatus;
        // Is the controller status initialized
        uint32_t m_initialized;
        // Previous controller states. Only valid if controller is connected
        XINPUT_STATE m_previousState;
    };
};
