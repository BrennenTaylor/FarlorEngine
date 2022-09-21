#include "InputStateManager.h"

#include "ControllerManager.h"

namespace Farlor {
const uint32_t LeftThumbDeadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE * 2;
const uint32_t RightThumbDeadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE * 2;
const uint32_t TriggerDeadzone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

UnidirectionalAbsoluteAxis::UnidirectionalAbsoluteAxis()
    : m_value(0.0f)
{
}

UnidirectionalAbsoluteAxis::UnidirectionalAbsoluteAxis(float normalizedFloatValue)
    : m_value(normalizedFloatValue)
{
}

UnidirectionalAbsoluteAxis::UnidirectionalAbsoluteAxis(float currentValue, float maxValue)
    : m_value(currentValue / maxValue)
{
}

UnidirectionalAbsoluteAxis::UnidirectionalAbsoluteAxis(int currentValue, int maxValue)
    : m_value((float)currentValue / (float)maxValue)
{
}

void UnidirectionalAbsoluteAxis::SetValueNormalized(float normalizedValue)
{
    m_value = normalizedValue;
}

void UnidirectionalAbsoluteAxis::SetValueUnnormalized(float currentValue, float maxValue)
{
    m_value = currentValue / maxValue;
}

void UnidirectionalAbsoluteAxis::SetValueDiscretized(int currentValue, int maxValue)
{
    m_value = (float)currentValue / (float)maxValue;
}

// Wrapper around a float value, values are valid between -1.0 and 1.0
BidirectionalAbsoluteAxis::BidirectionalAbsoluteAxis() { }

BidirectionalAbsoluteAxis::BidirectionalAbsoluteAxis(float normalizedFloatValue)
    : m_value(normalizedFloatValue)
{
}

BidirectionalAbsoluteAxis::BidirectionalAbsoluteAxis(float currentValue, float maxValue)
    : m_value(currentValue / maxValue)
{
}

BidirectionalAbsoluteAxis::BidirectionalAbsoluteAxis(int currentValue, int maxValue)
    : m_value((float)currentValue / (float)maxValue)
{
}


void BidirectionalAbsoluteAxis::SetValueNormalized(float normalizedValue)
{
    m_value = normalizedValue;
}

void BidirectionalAbsoluteAxis::SetValueUnnormalized(float currentValue, float maxValue)
{
    m_value = currentValue / maxValue;
}

void BidirectionalAbsoluteAxis::SetValueDiscretized(int currentValue, int maxValue)
{
    m_value = (float)currentValue / (float)maxValue;
}

void ButtonState::Reset()
{
    endedDown = false;
    numHalfSteps = 0;
}


void InputState::Clear()
{
    m_previousMousePos = Farlor::Vector2(0.0f, 0.0f);
    m_currentMousePos = Farlor::Vector2(0.0f, 0.0f);

    for (auto &mouseButton : m_mouseButtonStates) {
        mouseButton.Reset();
    }

    for (auto &key : m_keyboardButtonStates) {
        key.Reset();
    }
}

InputStateManager::InputStateManager()
    : m_inputStates()
    , m_controllerManager()
{
}

InputStateManager::~InputStateManager() { }

void InputStateManager::Clear()
{
    for (auto &is : m_inputStates) {
        is.Clear();
    }
}

void InputStateManager::Tick()
{
    // Rotate to the next input state
    m_writeInputStateIdx = (m_writeInputStateIdx + 1) % NumBufferedInputStates;
    m_readInputStateIdx = (m_readInputStateIdx + 1) % NumBufferedInputStates;

    InputState &writeInputState = m_inputStates[m_writeInputStateIdx];
    writeInputState.Clear();

    const InputState &readInputState = m_inputStates[m_readInputStateIdx];
    writeInputState.m_currentMousePos = readInputState.m_currentMousePos;
    writeInputState.m_previousMousePos = readInputState.m_currentMousePos;

    // Initialize current state
    for (uint32_t i = 0; i < MouseButtons::NumMouseButtons; ++i) {
        writeInputState.m_mouseButtonStates[i].endedDown
              = readInputState.m_mouseButtonStates[i].endedDown;
        writeInputState.m_mouseButtonStates[i].numHalfSteps = 0;
    }

    for (uint32_t i = 0; i < KeyboardButtons::NumKeyboardButtons; ++i) {
        writeInputState.m_keyboardButtonStates[i].endedDown
              = readInputState.m_keyboardButtonStates[i].endedDown;
        writeInputState.m_keyboardButtonStates[i].numHalfSteps = 0;
    }

    for (uint32_t i = 0; i < ControllerButtons::NumControllerButtons; ++i) {
        writeInputState.m_controllerState.m_controllerButtons[i].endedDown
              = readInputState.m_controllerState.m_controllerButtons[i].endedDown;
        writeInputState.m_controllerState.m_controllerButtons[i].numHalfSteps = 0;
    }

    PollControllers();
}

void InputStateManager::SetKeyboardState(uint32_t keyboardButtonIndex, bool isDown)
{
    InputState &writeInputState = m_inputStates[m_writeInputStateIdx];
    // writeInputState.m_latestInputType = InputType::KEYBOARD_MOUSE;
    // If we havent changed state, nothing to do
    if (writeInputState.m_keyboardButtonStates[keyboardButtonIndex].endedDown == isDown) {
        return;
    }
    // Else, flip the state
    writeInputState.m_keyboardButtonStates[keyboardButtonIndex].endedDown
          = !writeInputState.m_keyboardButtonStates[keyboardButtonIndex].endedDown;
    writeInputState.m_keyboardButtonStates[keyboardButtonIndex].numHalfSteps++;
}

void InputStateManager::SetMouseState(uint32_t mouseButtonIndex, bool isDown)
{
    InputState &writeInputState = m_inputStates[m_writeInputStateIdx];
    // writeInputState.m_latestInputType = InputType::KEYBOARD_MOUSE;
    // If we havent changed state, nothing to do
    if (writeInputState.m_mouseButtonStates[mouseButtonIndex].endedDown == isDown) {
        return;
    }
    // Else, flip the state
    writeInputState.m_mouseButtonStates[mouseButtonIndex].endedDown
          = !writeInputState.m_mouseButtonStates[mouseButtonIndex].endedDown;
    writeInputState.m_mouseButtonStates[mouseButtonIndex].numHalfSteps++;
}

void InputStateManager::SetMousePosition(int32_t mousePosX, int32_t mousePosY)
{
    InputState &writeInputState = m_inputStates[m_writeInputStateIdx];
    // writeInputState.m_latestInputType = InputType::KEYBOARD_MOUSE;
    writeInputState.m_currentMousePos
          = Farlor::Vector2(static_cast<float>(mousePosX), static_cast<float>(mousePosY));

    if (!m_currentMouseTracking) {
        writeInputState.m_previousMousePos = writeInputState.m_currentMousePos;
        m_currentMouseTracking = true;
    }
}

void InputStateManager::PollControllers()
{
    InputState &writeInputState = m_inputStates[m_writeInputStateIdx];
    m_controllerManager.Poll();

    if (!m_controllerManager.IsConnected()) {
        writeInputState.m_latestInputType = InputType::KEYBOARD_MOUSE;
        writeInputState.m_controllerState.m_leftJoystickXMovingAverage.Reset();
        writeInputState.m_controllerState.m_leftJoystickYMovingAverage.Reset();
        writeInputState.m_controllerState.m_rightJoystickXMovingAverage.Reset();
        writeInputState.m_controllerState.m_rightJoystickYMovingAverage.Reset();
        return;
    }

    if (m_controllerManager.IsConnected()) {
        writeInputState.m_latestInputType = InputType::CONTROLLER;

        writeInputState.m_controllerState.m_controllerType = ControllerType::XBOX_360;

        const XINPUT_STATE &latestState = m_controllerManager.AccessRawXInputState();

        // Left Stick Update
        {
            float LX = latestState.Gamepad.sThumbLX;
            float LY = latestState.Gamepad.sThumbLY;

            //determine how far the controller is pushed
            float magnitude = sqrt(LX * LX + LY * LY);

            //determine the direction the controller is pushed
            float normalizedLX = LX / magnitude;
            float normalizedLY = LY / magnitude;

            float normalizedMagnitude = 0;

            //check if the controller is outside a circular dead zone
            if (magnitude > (LeftThumbDeadzone)) {
                //clip the magnitude at its expected maximum value
                if (magnitude > 32767)
                    magnitude = 32767;

                //adjust magnitude relative to the end of the dead zone
                magnitude -= (LeftThumbDeadzone);

                //optionally normalize the magnitude with respect to its expected range
                //giving a magnitude value of 0.0 to 1.0
                normalizedMagnitude = magnitude / (32767 - (LeftThumbDeadzone));
            } else  //if the controller is in the deadzone zero out the magnitude
            {
                magnitude = 0.0;
                normalizedMagnitude = 0.0;
            }

            writeInputState.m_controllerState.m_leftJoystickXMovingAverage.AddSample(
                  LX * normalizedMagnitude);
            writeInputState.m_controllerState.m_leftJoystickYMovingAverage.AddSample(
                  LY * normalizedMagnitude);
        }

        // Right Stick Update
        {
            float RX = latestState.Gamepad.sThumbRX;
            float RY = latestState.Gamepad.sThumbRY;

            //determine how far the controller is pushed
            float magnitude = sqrt(RX * RX + RY * RY);

            //determine the direction the controller is pushed
            float normalizedLX = RX / magnitude;
            float normalizedLY = RY / magnitude;

            float normalizedMagnitude = 0;

            //check if the controller is outside a circular dead zone
            if (magnitude > (RightThumbDeadzone)) {
                //clip the magnitude at its expected maximum value
                if (magnitude > 32767)
                    magnitude = 32767;

                //adjust magnitude relative to the end of the dead zone
                magnitude -= (RightThumbDeadzone);

                //optionally normalize the magnitude with respect to its expected range
                //giving a magnitude value of 0.0 to 1.0
                normalizedMagnitude = magnitude / (32767 - (RightThumbDeadzone));
            } else  //if the controller is in the deadzone zero out the magnitude
            {
                magnitude = 0.0;
                normalizedMagnitude = 0.0;
            }

            writeInputState.m_controllerState.m_rightJoystickXMovingAverage.AddSample(
                  RX * normalizedMagnitude);
            writeInputState.m_controllerState.m_rightJoystickYMovingAverage.AddSample(
                  RY * normalizedMagnitude);
        }

        // Left Trigger Update
        {
            float triggerVal = (float)latestState.Gamepad.bLeftTrigger / 255.0f;
            writeInputState.m_controllerState.m_leftTrigger.AddSample(triggerVal);
        }

        // Right Trigger Update
        {
            float triggerVal = (float)latestState.Gamepad.bRightTrigger / 255.0f;
            writeInputState.m_controllerState.m_RightTrigger.AddSample(triggerVal);
        }

        // Add in the buttons
        auto ControllerButtonUpdate = [&, this](const ControllerButtons button,
                                            const uint32_t xinputVal) {
            if (writeInputState.m_controllerState.m_controllerButtons[button].endedDown
                  == (latestState.Gamepad.wButtons & xinputVal ? true : false)) {
                return;
            }
            // Else, flip the state
            writeInputState.m_controllerState.m_controllerButtons[button].endedDown
                  = !writeInputState.m_controllerState.m_controllerButtons[button].endedDown;
            writeInputState.m_controllerState.m_controllerButtons[button].numHalfSteps++;
        };

        ControllerButtonUpdate(ControllerButtons::DPadUp, XINPUT_GAMEPAD_DPAD_UP);
        ControllerButtonUpdate(ControllerButtons::DPadDown, XINPUT_GAMEPAD_DPAD_DOWN);
        ControllerButtonUpdate(ControllerButtons::DPadLeft, XINPUT_GAMEPAD_DPAD_LEFT);
        ControllerButtonUpdate(ControllerButtons::DPadRight, XINPUT_GAMEPAD_DPAD_RIGHT);

        ControllerButtonUpdate(ControllerButtons::Start, XINPUT_GAMEPAD_START);
        ControllerButtonUpdate(ControllerButtons::Select, XINPUT_GAMEPAD_BACK);

        ControllerButtonUpdate(ControllerButtons::LeftThumb, XINPUT_GAMEPAD_LEFT_THUMB);
        ControllerButtonUpdate(ControllerButtons::RightThumb, XINPUT_GAMEPAD_RIGHT_THUMB);

        ControllerButtonUpdate(ControllerButtons::LeftBumper, XINPUT_GAMEPAD_LEFT_SHOULDER);
        ControllerButtonUpdate(ControllerButtons::RightBumper, XINPUT_GAMEPAD_RIGHT_SHOULDER);

        ControllerButtonUpdate(ControllerButtons::A, XINPUT_GAMEPAD_A);
        ControllerButtonUpdate(ControllerButtons::B, XINPUT_GAMEPAD_B);
        ControllerButtonUpdate(ControllerButtons::X, XINPUT_GAMEPAD_X);
        ControllerButtonUpdate(ControllerButtons::Y, XINPUT_GAMEPAD_Y);
    }
}
}