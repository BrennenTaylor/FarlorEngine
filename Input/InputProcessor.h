#pragma once

namespace Farlor
{
    class InputProcessor
    {
    public:
        InputProcessor();
        ~InputProcessor();

        // This will tick each frame and allow the processor to handle any per-frame updates
        void Process();

    private:
        
    };
}