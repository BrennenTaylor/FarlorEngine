#pragma once

#include "../Input/InputStateManager.h"
#include "../NewRenderer/Renderer.h"
#include "../NewRenderer/Camera.h"
#include "../Physics/PhysicsSystem.h"

#include "Win32Window.h"

#include "EngineConfig.h"

#include "Timer.h"

#include <deque>

namespace Farlor {
struct InputState;

class Engine {
   public:
    Engine();
    void Initialize(EngineConfig config);

    int Run();

    inline InputStateManager &AccessInputStateManager() { return m_inputStateManager; }

    inline void TriggerShutdown() { m_running = false; }

    inline const std::deque<float> &GetFrameTimes() const { return m_frameTimes; }

   private:
    void HandleInput(const InputState &latestInputState);

   private:
    bool m_fullscreen = false;
    bool m_vsync = true;

    bool m_running = true;

    Farlor::Timer m_timerMaster;

    std::deque<float> m_frameTimes;

    GameWindow m_gameWindow;
    InputStateManager m_inputStateManager;
    Renderer m_renderingSystem;
    PhysicsSystem m_physicsSystem;
    std::array<Camera, 2> m_cameras;
    uint32_t m_currentCameraIdx = 0;
};
}
