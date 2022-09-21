#pragma once

#include "ControllerButton.h"
#include "KeyboardButton.h"
#include "MouseButton.h"

#include "ControllerManager.h"

#include <FMath/FMath.h>

#include <vector>

#include <array>
#include <memory>

class ControllerManager;

constexpr uint32_t NumBufferedInputStates = 5;

namespace Farlor {
template<typename Type, uint32_t size>
class MovingAverage {
    std::array<Type, size> m_samples;
    Type m_sum = 0;
    uint32_t m_currentSample = 0;
    ;
    uint32_t m_sampleCount = 0;

   public:
    MovingAverage() { }

    void Reset()
    {
        m_sum = 0;
        m_currentSample = 0;
        m_sampleCount = 0;
    }

    void AddSample(Type value)
    {
        if (m_sampleCount == size) {
            m_sum -= m_samples[m_currentSample];
        } else {
            m_sampleCount++;
        }

        m_samples[m_currentSample] = value;
        m_sum += value;
        m_currentSample = (m_currentSample + 1) % size;
    }

    float GetCurrentAvereage() const
    {
        if (m_sampleCount == 0)
            return 0.0f;

        return static_cast<float>(m_sum) / static_cast<float>(m_sampleCount);
    }
};

// Wrapper around a float value, values are valid between 0.0 and 1.0
class UnidirectionalAbsoluteAxis {
   public:
    UnidirectionalAbsoluteAxis();
    UnidirectionalAbsoluteAxis(float normalizedFloatValue);
    UnidirectionalAbsoluteAxis(float currentValue, float maxValue);
    UnidirectionalAbsoluteAxis(int currentValue, int maxValue);

    float GetValue() const { return m_value; }

    void SetValueNormalized(float normalizedValue);
    void SetValueUnnormalized(float currentValue, float maxValue);
    void SetValueDiscretized(int currentValue, int maxValue);

   private:
    float m_value;
};

// Wrapper around a float value, values are valid between -1.0 and 1.0
class BidirectionalAbsoluteAxis {
   public:
    BidirectionalAbsoluteAxis();
    BidirectionalAbsoluteAxis(float normalizedFloatValue);
    BidirectionalAbsoluteAxis(float currentValue, float maxValue);
    BidirectionalAbsoluteAxis(int currentValue, int maxValue);

    float GetValue() const { return m_value; }

    void SetValueNormalized(float normalizedValue);
    void SetValueUnnormalized(float currentValue, float maxValue);
    void SetValueDiscretized(int currentValue, int maxValue);

   private:
    float m_value;
};

enum InputType { KEYBOARD_MOUSE = 0, CONTROLLER = 1 };

enum ControllerType { UNKNOWN = 0, DUALSHOCK_4 = 1, XBOX_360 = 2 };

struct ButtonState {
    bool endedDown = false;
    uint32_t numHalfSteps = 0;

    void Reset();
};

struct ControllerState {
    std::array<ButtonState, ControllerButtons::NumControllerButtons> m_controllerButtons;

    MovingAverage<float, 3> m_leftTrigger;
    MovingAverage<float, 3> m_RightTrigger;

    MovingAverage<float, 3> m_leftJoystickXMovingAverage;
    MovingAverage<float, 3> m_leftJoystickYMovingAverage;

    MovingAverage<float, 3> m_rightJoystickXMovingAverage;
    MovingAverage<float, 3> m_rightJoystickYMovingAverage;

    ControllerType m_controllerType = ControllerType::UNKNOWN;
};

struct InputState {
    // Default to using keyboard and mouse
    InputType m_latestInputType = InputType::KEYBOARD_MOUSE;

    // Mouse state
    Farlor::Vector2 m_previousMousePos;
    Farlor::Vector2 m_currentMousePos;

    std::array<ButtonState, MouseButtons::NumMouseButtons> m_mouseButtonStates;

    // Keyboard State
    std::array<ButtonState, KeyboardButtons::NumKeyboardButtons> m_keyboardButtonStates;

    // Simply allow one controller atm
    ControllerState m_controllerState;

    void Clear();
};

class InputStateManager {
   public:
    InputStateManager();
    ~InputStateManager();

    void Clear();

    // This returns a structure containing all the input we need since the last frame
    const InputState &GetLatestInputState() { return m_inputStates[m_readInputStateIdx]; }

    // Called once per frame
    void Tick();

    // Update Input States
    void SetKeyboardState(uint32_t keyboardButtonIndex, bool isDown);
    void SetMouseState(uint32_t mouseButtonIndex, bool isDown);

    void SetMousePosition(int32_t deltaX, int32_t deltaY);

   private:
    void PollControllers();

   private:
    std::array<InputState, NumBufferedInputStates> m_inputStates;
    uint32_t m_writeInputStateIdx = 0;
    uint32_t m_readInputStateIdx = NumBufferedInputStates - 1;

    ControllerManager m_controllerManager;

    bool m_currentMouseTracking = false;
};
}