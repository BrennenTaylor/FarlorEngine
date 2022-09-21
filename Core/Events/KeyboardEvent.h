#pragma once

#include "EventData.h"

#include "../../Input/Keys.h"

#include <cstdint>

namespace Farlor
{
    struct KeyboardEvent : public EventData
    {
        KeyboardEvent(Keys keyPressed, uint32_t pressed);

        EventType GetEventType() override;

        static const EventType m_eventType;
        Keys m_keyPressed;
        uint32_t m_pressed;
    };

}
