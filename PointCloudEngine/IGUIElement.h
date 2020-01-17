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

		void SetWindowFontStyleMessage(HWND hwnd)
		{
			SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		}

		static void InitializeFontHandle()
		{
			// Retrieve font from windows messages (better than default font)
			NONCLIENTMETRICS metrics;
			metrics.cbSize = sizeof(NONCLIENTMETRICS);
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0);
			hFont = CreateFontIndirect(&metrics.lfMessageFont);
		}

		static void DeleteFontHandle()
		{
			DeleteObject(hFont);
		}
	};
}
#endif
