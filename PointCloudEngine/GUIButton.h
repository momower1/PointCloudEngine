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
		std::function<void()> OnClick = NULL;

		GUIButton(HWND hwndParent, int positionX, int positionY, int width, int height, std::wstring name, std::function<void()> OnClick)
		{
			// Create the button
			this->OnClick = OnClick;
			hwndElement = CreateWindowEx(NULL, L"BUTTON", name.c_str(), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, positionX, positionY, width, height, hwndParent, NULL, NULL, NULL);
			IGUIElement::SetCustomWindowFontStyle(hwndElement);
		}

		void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (msg == WM_COMMAND)
			{
				if (((HWND)lParam == hwndElement) && (HIWORD(wParam) == BN_CLICKED) && OnClick != NULL)
				{
					OnClick();
				}
			}
		}
	};
}
#endif