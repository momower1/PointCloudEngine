#include "Input.h"

Vector2 Input::mouseDelta;
Vector2 Input::mousePosition;

Keyboard Input::keyboard;
Keyboard::State Input::keyboardState;
Keyboard::KeyboardStateTracker Input::keyboardStateTracker;

Mouse Input::mouse;
Mouse::State Input::mouseState;
Mouse::ButtonStateTracker Input::mouseButtonStateTracker;

float Input::mouseScrollDelta;
Vector2 Input::rawMouseMovement;

void Input::Initialize(HWND hwnd)
{
    // Basic mouse input and absolute position
    mouse.SetWindow(hwnd);
    mouse.SetMode(Mouse::MODE_RELATIVE);
    mouseButtonStateTracker.Reset();

    // Set position
    mousePosition.x = settings->resolutionX / 2;
    mousePosition.y = settings->resolutionY / 2;

    // Keyboard
    keyboard.Reset();
    keyboardStateTracker.Reset();

    // Raw mouse input (for first-person camera) for relative position
    RAWINPUTDEVICE rawMouseInputDevice;
    rawMouseInputDevice.usUsagePage = (USHORT)0x01;                         // HID_USAGE_PAGE_GENERIC
    rawMouseInputDevice.usUsage = (USHORT)0x02;                             //HID_USAGE_GENERIC_MOUSE;
    rawMouseInputDevice.dwFlags = RIDEV_INPUTSINK;
    rawMouseInputDevice.hwndTarget = hwnd;

    if (!RegisterRawInputDevices(&rawMouseInputDevice, 1, sizeof(RAWINPUTDEVICE)))
    {
		ERROR_MESSAGE(NAMEOF(RegisterRawInputDevices) + L" failed!");
    }
}

void Input::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    keyboard.ProcessMessage(msg, wParam, lParam);
    mouse.ProcessMessage(msg, wParam, lParam);

    if (msg == WM_INPUT)
    {
        UINT dwSize = 0;

        // Determine the size of the input data
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

        BYTE *data = new BYTE[dwSize];

        // Get the actual input data
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, data, &dwSize, sizeof(RAWINPUTHEADER));

        RAWINPUT* raw = (RAWINPUT*)data;

        if (raw->header.dwType == RIM_TYPEMOUSE)
        {
            rawMouseMovement.x += raw->data.mouse.lLastX;
            rawMouseMovement.y += raw->data.mouse.lLastY;
        }

        delete[] data;
    }
}

void Input::Update()
{
    keyboardState = keyboard.GetState();
    mouseState = mouse.GetState();

    keyboardStateTracker.Update(keyboardState);
    mouseButtonStateTracker.Update(mouseState);

    // Save mouse scroll delta
    mouseScrollDelta = 0.01f * settings->scrollSensitivity * mouseState.scrollWheelValue;
    mouse.ResetScrollWheelValue();

    // For system wide mouse position set the mouse mode to absolute and get it from the mouse state (drawback: mouse can move out of the window)
    mousePosition.x = max(0, min(mousePosition.x + rawMouseMovement.x, settings->resolutionX));
    mousePosition.y = max(0, min(mousePosition.y + rawMouseMovement.y, settings->resolutionY));
    mouseDelta = settings->mouseSensitivity * rawMouseMovement;

    // Reset delta value
    rawMouseMovement = Vector2(0, 0);
}

bool Input::GetKey(Keyboard::Keys key)
{
    return keyboardState.IsKeyDown(key);
}

bool Input::GetKeyUp(Keyboard::Keys key)
{
    return keyboardStateTracker.IsKeyReleased(key);
}

bool Input::GetKeyDown(Keyboard::Keys key)
{
    return keyboardStateTracker.IsKeyPressed(key);
}

bool Input::GetMouseButton(MouseButton::MouseButton button)
{
    switch (button)
    {
        case MouseButton::LeftButton:
        {
            return mouseState.leftButton;
        }

        case MouseButton::RightButton:
        {
            return mouseState.rightButton;
        }

        case MouseButton::MiddleButton:
        {
            return mouseState.middleButton;
        }

        case MouseButton::xButton1:
        {
            return mouseState.xButton1;
        }

        case MouseButton::xButton2:
        {
            return mouseState.xButton2;
        }
    }

    return false;
}

bool Input::GetMouseButtonUp(MouseButton::MouseButton button)
{
    switch (button)
    {
        case MouseButton::LeftButton:
        {
            return mouseButtonStateTracker.leftButton == Mouse::ButtonStateTracker::RELEASED;
        }

        case MouseButton::RightButton:
        {
            return mouseButtonStateTracker.rightButton == Mouse::ButtonStateTracker::RELEASED;
        }

        case MouseButton::MiddleButton:
        {
            return mouseButtonStateTracker.middleButton == Mouse::ButtonStateTracker::RELEASED;
        }

        case MouseButton::xButton1:
        {
            return mouseButtonStateTracker.xButton1 == Mouse::ButtonStateTracker::RELEASED;
        }

        case MouseButton::xButton2:
        {
            return mouseButtonStateTracker.xButton2 == Mouse::ButtonStateTracker::RELEASED;
        }
    }

    return false;
}

bool Input::GetMouseButtonDown(MouseButton::MouseButton button)
{
    switch (button)
    {
        case MouseButton::LeftButton:
        {
            return mouseButtonStateTracker.leftButton == Mouse::ButtonStateTracker::PRESSED;
        }

        case MouseButton::RightButton:
        {
            return mouseButtonStateTracker.rightButton == Mouse::ButtonStateTracker::PRESSED;
        }

        case MouseButton::MiddleButton:
        {
            return mouseButtonStateTracker.middleButton == Mouse::ButtonStateTracker::PRESSED;
        }

        case MouseButton::xButton1:
        {
            return mouseButtonStateTracker.xButton1 == Mouse::ButtonStateTracker::PRESSED;
        }

        case MouseButton::xButton2:
        {
            return mouseButtonStateTracker.xButton2 == Mouse::ButtonStateTracker::PRESSED;
        }
    }

    return false;
}

void Input::SetSensitivity(float mouseSensitivity, float scrollSensitivity)
{
    settings->mouseSensitivity = mouseSensitivity;
    settings->scrollSensitivity = scrollSensitivity;
}

void Input::SetMode(Mouse::Mode mode)
{
    // Relative mode will also make the mouse invisible, absolute mode will make it visible
    Input::mouse.SetMode(mode);
}
