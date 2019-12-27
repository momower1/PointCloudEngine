#ifndef GUIBUTTON_H
#define GUIBUTTON_H

#pragma once
#include "PointCloudEngine.h"
#include "IGUIElement.h"

namespace PointCloudEngine
{
	class GUIButton : public IGUIElement
	{
	public:
		HWND hwndButton = NULL;
		std::function<void()> OnClick = NULL;

		GUIButton(HWND hwndParent, XMUINT2 pos, XMUINT2 size, std::wstring name, std::function<void()> OnClick)
		{
			// Create the button
			this->OnClick = OnClick;
			hwndButton = CreateWindowEx(NULL, L"BUTTON", name.c_str(), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, pos.x, pos.y, size.x, size.y, hwndParent, NULL, NULL, NULL);
		}

		void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (msg == WM_COMMAND)
			{
				if (((HWND)lParam == hwndButton) && (HIWORD(wParam) == BN_CLICKED))
				{
					OnClick();
				}
			}
		}

		void Show(int SW_COMMAND)
		{
			ShowWindow(hwndButton, SW_COMMAND);
		}
	};
}
#endif