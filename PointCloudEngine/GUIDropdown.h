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
		int* value = NULL;
		std::function<void()> OnSelect = NULL;
		std::vector<std::wstring> entries;

		GUIDropdown(HWND hwndParent, int positionX, int positionY, int width, int height, std::initializer_list<std::wstring> entries, std::function<void()> OnSelect, int* value)
		{
			// Create the dropdown menu
			this->value = value;
			this->OnSelect = OnSelect;
			hwndElement = CreateWindowEx(NULL, WC_COMBOBOX, L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE, positionX, positionY, width, height, hwndParent, NULL, NULL, NULL);
			IGUIElement::SetCustomWindowFontStyle(hwndElement);
			SetEntries(entries);
		}

		void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (msg == WM_COMMAND)
			{
				if (((HWND)lParam == hwndElement) && (HIWORD(wParam) == CBN_SELCHANGE))
				{
					// Invoke the function
					*value = SendMessage(hwndElement, CB_GETCURSEL, 0, 0);

					if (OnSelect != NULL)
					{
						OnSelect();
					}
				}
			}
		}

		void SetEntries(std::vector<std::wstring> entries)
		{
			this->entries = entries;

			// Clear the dropdown
			SendMessage(hwndElement, CB_RESETCONTENT, 0, 0);

			// Add items to the dropdown
			for (auto it = entries.begin(); it != entries.end(); it++)
			{
				SendMessage(hwndElement, CB_ADDSTRING, 0, (LPARAM)it->c_str());
			}

			// Set initial value
			SendMessage(hwndElement, CB_SETCURSEL, *value, 0);
		}

		std::wstring GetSelectedString()
		{
			return entries[*value];
		}

		void SetSelection(int selection)
		{
			*value = selection;
			SendMessage(hwndElement, CB_SETCURSEL, selection, 0);
		}

		void Show(int SW_COMMAND)
		{
			IGUIElement::Show(SW_COMMAND);
			SendMessage(hwndElement, CB_SETCURSEL, *value, 0);
		}
	};
}
#endif