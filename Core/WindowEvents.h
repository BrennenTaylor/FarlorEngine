#pragma once

#include "MouseButtons.h"
#include "KeyboardButtons.h"

#include <cstdint>

namespace Farlor
{
    // Define all the structs here for the window types
    struct WindowResizeEventArgs
    {
        uint32_t newWidth;
        uint32_t newHeight;
    };

    struct WindowClosedEventArgs
    {
    };


    // Mouse Events
    struct MouseMoveEventArgs
    {
        int32_t xPos;
        int32_t yPos;
    };

    struct MouseButtonEventArgs
    {
        WindowMouseButtons button;
        WindowMouseButtonState buttonState;
    };

    // Keyboard Events
    struct KeyboardButtonEventArgs
    {
        WindowKeyboardButton button;
        WindowKeyboardButtonState buttonState;
    };

    // Enum of event types
    enum class WindowEventType
    {
        WindowResizeEvent,
        WindowClosedEvent,
        MouseMoveEvent,
        MouseButtonEvent,
        KeyboardButtonEvent
    };

/////////////////// The big ol window event class

    struct WindowEvent
    {
        WindowEventType type;

        union
        {
            // Window Events
            WindowResizeEventArgs windowResizeEventArgs;
            WindowClosedEventArgs windowClosedEventArgs;

            // Mouse events
            MouseMoveEventArgs mouseMoveEventArgs;
            MouseButtonEventArgs mouseButtonEventArgs;

            // Keyboard events
            KeyboardButtonEventArgs keyboardButtonEventArgs;

        } WindowEventArgs;
    };
}