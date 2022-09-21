#pragma once

#include <cstdint>

namespace Farlor
{
    enum class WindowKeyboardButton : uint32_t
    {
        // Special
        Backspace,
        Tab,
        Return,
        Shift,
        Ctrl,
        Alt,
        Space,
        Escape,

        // Arrow Keys
        LeftArrow,
        UpArrow,
        RightArrow,
        DownArrow,

        // Numbers
        Zero,
        One,
        Two,
        Three,
        Four,
        Five,
        Six,
        Seven,
        Eight,
        Nine,

        // Alphabet
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        Tilda
    };

    enum class WindowKeyboardButtonState : uint32_t
    {
        Down = 0u,
        Up = 1u
    };
}