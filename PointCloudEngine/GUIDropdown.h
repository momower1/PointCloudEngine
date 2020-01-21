#ifndef GUIDROPDOWN_H
#define GUIDROPDOWN_H

#pragma once
#include "PointCloudEngine.h"
#include "IGUIElement.h"

namespace PointCloudEngine
{
	class GUIDropdown : public IGUIElement
	{
	public:
		XMUINT2 size;
		int* value = NULL;
		HWND hwndDropdown = NULL;
		std::function<void()> OnSelect = NULL;

		GUIDropdown(HWND hwndParent, XMUINT2 pos, XMUINT2 size, std::initializer_list<std::wstring> entries, std::function<void()> OnSelect, int* value)
		{
			// Create the dropdown menu
			this->size = size;
			this->value = value;
			this->OnSelect = OnSelect;
			hwndDropdown = CreateWindowEx(NULL, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE, pos.x, pos.y, size.x, size.y, hwndParent, NULL, NULL, NULL);
			SetCustomWindowFontStyle(hwndDropdown);

			// Add items to the dropdown
			for (auto it = entries.begin(); it != entries.end(); it++)
			{
				SendMessage(hwndDropdown, CB_ADDSTRING, 0, (LPARAM)it->c_str());
			}

			// Set initial value
			SendMessage(hwndDropdown, CB_SETCURSEL, *value, 0);
		}

		void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (msg == WM_COMMAND)
			{
				if (((HWND)lParam == hwndDropdown) && (HIWORD(wParam) == CBN_SELCHANGE))
				{
					// Invoke the function
					*value = SendMessage(hwndDropdown, CB_GETCURSEL, 0, 0);
					OnSelect();
				}
			}
		}

		void SetPosition(XMUINT2 position)
		{
			MoveWindow(hwndDropdown, position.x, position.y, size.x, size.y, true);
		}

		void SetSelection(int selection)
		{
			*value = selection;
			SendMessage(hwndDropdown, CB_SETCURSEL, selection, 0);
		}

		void Show(int SW_COMMAND)
		{
			ShowWindow(hwndDropdown, SW_COMMAND);
			SendMessage(hwndDropdown, CB_SETCURSEL, *value, 0);
		}
	};
}
#endif