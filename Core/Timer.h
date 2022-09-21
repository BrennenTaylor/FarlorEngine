#pragma once

#include <cstdint>

namespace Farlor {
class Timer {
   public:
    Timer();

    float DeltaTime() const;
    // In seconds
    float TotalTime() const;

    void Reset();
    void Start();
    void Stop();
    void Tick();

    void Toggle();

   private:
    double m_secondsPerCount = 0;
    double m_deltaTime = 0.0;

    int64_t m_baseTime = 0;
    int64_t m_pausedTime = 0;
    int64_t m_stopTime = 0;
    int64_t m_prevTime = 0;
    int64_t m_currentTime = 0;

    bool m_stopped = true;
};
}
