#pragma once

#include "Window.h"

#include "WindowEvents.h"

#include "..\Platform\Platform.h"

#include <FMath/FMath.h>

#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <functional>

namespace Farlor {
class Engine;

template<>
class Window<PlatformType::Win32> : public WindowBase<Window<PlatformType::Win32>> {
   public:
    friend class WindowBase<Window<PlatformType::Win32>>;

    Window(Engine &engine);
    bool Initialize(bool fullscreen);
    void ProcessSystemMessages();
    void Shutdown();

    void ShowGameWindow();
    void ShowGameWindowCursor();

    void SetWindowTitleText(std::string windowText);

    Farlor::Vector2 GetWindowCenter() const;
    void SetCursorPos(uint32_t x, uint32_t y);

    HWND GetWindowHandle() const { return m_windowHandle; }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

   private:
    LRESULT CALLBACK RealWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void WindowEventManager(WindowEvent *pEvent);

   public:
    unsigned int m_clientWidth = 0;
    unsigned int m_clientHeight = 0;

    unsigned int m_windowWidth = 0;
    unsigned int m_windowHeight = 0;

   private:
    Engine &m_engine;

   protected:
    bool m_isRunning = false;
    HWND m_windowHandle = NULL;
    bool m_fullscreen = false;

    std::function<void(WindowEvent *WindowEvent)> m_windowEventCallback;
};
}
