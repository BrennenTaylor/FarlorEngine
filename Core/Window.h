#pragma once

#include "..\Platform\Platform.h"

// #ifdef WIN32
// #include <windows.h>
// typedef HWND WindowHandle;
// #else
// typedef void* WindowHandle;
// #endif

namespace Farlor {
template<typename Window>
class WindowBase {
   public:
    unsigned int GetHeight() const { return static_cast<const Window *>(this)->m_clientHeight; }
    unsigned int GetWidth() const { return static_cast<const Window *>(this)->m_clientWidth; }

    bool IsRunning() const { return static_cast<const Window *>(this)->m_isRunning; }

    bool IsFullscreen() const { return static_cast<const Window *>(this)->m_fullscreen; }
};

template<PlatformType p>
class Window {
};
}

#if defined(_WIN32) || defined(WIN32)
#include "Win32Window.h"

namespace Farlor {
typedef Farlor::Window<Farlor::PlatformType::Win32> GameWindow;
}
#endif