#ifndef IGUIELEMENT_H
#define IGUIELEMENT_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
	class IGUIElement
	{
	public:
		// Needs to be declared in some cpp file (GUI.cpp)
		static HFONT hFont;

		// The window associated with the GUI element
		HWND hwndElement = NULL;

		virtual void Update() {}
		virtual void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {}
		
		virtual void SetPosition(int positionX, int positionY)
		{
			RECT rect;

			if (GetClientRect(hwndElement, &rect))
			{
				MoveWindow(hwndElement, positionX, positionY, rect.right - rect.left, rect.bottom - rect.top, true);
			}
		}
		
		virtual void Show(int SW_COMMAND)
		{
			ShowWindow(hwndElement, SW_COMMAND);
		}

		static void SetCustomWindowFontStyle(HWND hwnd)
		{
			SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		}

		template <typename T>
		static std::wstring ToPrecisionWstring(const T value, const int prescision = 6)
		{
			std::wostringstream s;
			s.precision(prescision);
			s << std::fixed << value;
			return s.str();
		}

		static void InitializeFontHandle(int fontSize)
		{
			// Better than default font, also should be consistent size
			hFont = CreateFont(fontSize, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
		}

		static void DeleteFontHandle()
		{
			DeleteObject(hFont);
		}
	};
}
#endif
