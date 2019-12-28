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
		XMUINT2 size;
		HWND hwndCheckbox = NULL;
		std::function<void(bool)> OnClick = NULL;

		GUICheckbox(HWND hwndParent, XMUINT2 pos, XMUINT2 size, std::wstring name, std::function<void(bool)> OnClick, bool initialChecked = false)
		{
			// Create the checkbox
			this->size = size;
			this->OnClick = OnClick;
			hwndCheckbox = CreateWindowEx(NULL, L"BUTTON", name.c_str(), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, pos.x, pos.y, size.x, size.y, hwndParent, NULL, NULL, NULL);

			// Set initial value
			SendMessage(hwndCheckbox, BM_SETCHECK, initialChecked ? BST_CHECKED : BST_UNCHECKED, 0);
		}

		void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (msg == WM_COMMAND)
			{
				if (((HWND)lParam == hwndCheckbox) && (HIWORD(wParam) == BN_CLICKED))
				{
					OnClick(SendMessage(hwndCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
				}
			}
		}

		void SetPosition(XMUINT2 position)
		{
			MoveWindow(hwndCheckbox, position.x, position.y, size.x, size.y, true);
		}

		void Show(int SW_COMMAND)
		{
			ShowWindow(hwndCheckbox, SW_COMMAND);
		}
	};
}
#endif