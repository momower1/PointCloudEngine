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

		virtual void Update() {}
		virtual void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {}
		virtual void SetPosition(XMUINT2 position) = 0;
		virtual void Show(int SW_COMMAND) = 0;

		void SetCustomWindowFontStyle(HWND hwnd)
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

		static void InitializeFontHandle()
		{
			// Better than default font, also should be consistent size
			hFont = CreateFont(20, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
		}

		static void DeleteFontHandle()
		{
			DeleteObject(hFont);
		}
	};
}
#endif
