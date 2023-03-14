#ifndef GUICHECKBOX_H
#define GUICHECKBOX_H

#pragma once
#include "PointCloudEngine.h"
#include "IGUIElement.h"

namespace PointCloudEngine
{
	class GUICheckbox : public IGUIElement
	{
	public:
		bool* value = NULL;
		std::function<void()> OnClick = NULL;

		GUICheckbox(HWND hwndParent, int positionX, int positionY, int width, int height, std::wstring name, std::function<void()> OnClick, bool *value)
		{
			// Create the checkbox
			this->value = value;
			this->OnClick = OnClick;
			hwndElement = CreateWindowEx(NULL, WC_BUTTON, name.c_str(), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, positionX, positionY, width, height, hwndParent, NULL, NULL, NULL);

			// Set initial value
			SendMessage(hwndElement, BM_SETCHECK, *value ? BST_CHECKED : BST_UNCHECKED, 0);
		}

		void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (msg == WM_COMMAND)
			{
				if (((HWND)lParam == hwndElement) && (HIWORD(wParam) == BN_CLICKED))
				{
					*value = SendMessage(hwndElement, BM_GETCHECK, 0, 0) == BST_CHECKED;

					if (OnClick != NULL)
					{
						OnClick();
					}
				}
			}
		}
	};
}
#endif