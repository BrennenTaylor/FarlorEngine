#pragma once

#include "EventData.h"

#include <cstdint>

namespace Farlor
{
    struct ControllerConnectionEvent : public EventData
    {
        ControllerConnectionEvent(uint32_t controllerNum, uint32_t connected);

        EventType GetEventType() override;

        static const EventType m_eventType;
        uint32_t m_controllerNum;
        uint32_t m_connected;
    };

}
