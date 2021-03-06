#ifndef INPUT_H
#define INPUT_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    namespace MouseButton
    {
        enum MouseButton
        {
            LeftButton,
            MiddleButton,
            RightButton,
            xButton1,
            xButton2
        };
    }

    class Input
    {
    public:
        static void Initialize (HWND hwnd);
        static void HandleMessage (UINT msg, WPARAM wParam, LPARAM lParam);
        static void Update ();

        static bool GetKey (Keyboard::Keys key);
        static bool GetKeyUp(Keyboard::Keys key);
        static bool GetKeyDown (Keyboard::Keys key);

        static bool GetMouseButton(MouseButton::MouseButton button);
        static bool GetMouseButtonUp(MouseButton::MouseButton button);
        static bool GetMouseButtonDown(MouseButton::MouseButton button);

        static void SetSensitivity(float mouseSensitivity, float scrollSensitivity);
        static void SetMode(Mouse::Mode mode);

        static Vector2 mouseDelta;
        static Vector2 mousePosition;
        static float mouseScrollDelta;

    private:
        static Keyboard keyboard;
        static Keyboard::State keyboardState;
        static Keyboard::KeyboardStateTracker keyboardStateTracker;

        static Mouse mouse;
        static Mouse::State mouseState;
        static Mouse::ButtonStateTracker mouseButtonStateTracker;

        static Vector2 rawMouseMovement;
    };
}

#endif
