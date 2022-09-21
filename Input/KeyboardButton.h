#pragma once

#include <cstdint>

namespace Farlor
{
    // These are indices into the input state arrays storing key state
    namespace KeyboardButtons
    {
        // Special
        const uint32_t Backspace = 0;
        const uint32_t Tab = Backspace + 1;
        const uint32_t Return = Tab + 1;
        const uint32_t Shift = Return + 1;
        const uint32_t Ctrl = Shift + 1;
        const uint32_t Alt = Ctrl + 1;
        const uint32_t Space = Alt + 1;
        const uint32_t Escape = Space + 1;

        // Arrow Keys
        const uint32_t LeftArrow = Escape + 1;
        const uint32_t UpArrow = LeftArrow + 1;
        const uint32_t RightArrow = UpArrow + 1;
        const uint32_t DownArrow = RightArrow + 1;

        // Numbers
        const uint32_t Zero = DownArrow + 1;
        const uint32_t One = Zero + 1;
        const uint32_t Two = One + 1;
        const uint32_t Three = Two + 1;
        const uint32_t Four = Three + 1;
        const uint32_t Five = Four + 1;
        const uint32_t Six = Five + 1;
        const uint32_t Seven = Six + 1;
        const uint32_t Eight = Seven + 1;
        const uint32_t Nine = Eight + 1;

        // Alphabet
        const uint32_t A = Nine + 1;
        const uint32_t B = A + 1;
        const uint32_t C = B + 1;
        const uint32_t D = C + 1;
        const uint32_t E = D + 1;
        const uint32_t F = E + 1;
        const uint32_t G = F + 1;
        const uint32_t H = G + 1;
        const uint32_t I = H + 1;
        const uint32_t J = I + 1;
        const uint32_t K = J + 1;
        const uint32_t L = K + 1;
        const uint32_t M = L + 1;
        const uint32_t N = M + 1;
        const uint32_t O = N + 1;
        const uint32_t P = O + 1;
        const uint32_t Q = P + 1;
        const uint32_t R = Q + 1;
        const uint32_t S = R + 1;
        const uint32_t T = S + 1;
        const uint32_t U = T + 1;
        const uint32_t V = U + 1;
        const uint32_t W = V + 1;
        const uint32_t X = W + 1;
        const uint32_t Y = X + 1;
        const uint32_t Z = Y + 1;

        const uint32_t Tilda = Z + 1;
        
        const uint32_t NumKeyboardButtons = Tilda + 1;
    }
}
