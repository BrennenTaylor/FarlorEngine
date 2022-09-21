#include "ControllerConnectionEvent.h"

namespace Farlor
{
    const EventType ControllerConnectionEvent::m_eventType("controller_connection_event");

    ControllerConnectionEvent::ControllerConnectionEvent(uint32_t controllerNum, uint32_t connected)
    {
        m_controllerNum = controllerNum;
        m_connected = connected;
    }

    EventType ControllerConnectionEvent::GetEventType()
    {
        return m_eventType;
    }
}
