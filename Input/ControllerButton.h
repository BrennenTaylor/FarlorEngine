#pragma once

#include <cstdint>

namespace Farlor
{
    const uint32_t MaxNumControllers = 4;
    enum ControllerButtons
    {
        X = 0,
        Y = X + 1,
        B = Y + 1,
        A = B + 1,
        Select = A + 1,
        Start = Select + 1,
        DPadLeft = Start + 1,
        DPadUp = DPadLeft + 1,
        DPadRight = DPadUp + 1,
        DPadDown = DPadRight + 1,
        LeftBumper = DPadDown + 1,
        RightBumper = LeftBumper + 1,
        LeftThumb = RightBumper + 1,
        RightThumb = LeftThumb + 1,
        NumControllerButtons = RightThumb + 1
    };

    namespace Controllers
    {
        const uint32_t Controller0 = 0;
        const uint32_t Controller1 = 1;
        const uint32_t Controller2 = 2;
        const uint32_t Controller3 = 3;
    };
}