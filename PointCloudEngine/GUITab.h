#ifndef GUITAB_H
#define GUITAB_H

#pragma once
#include "PointCloudEngine.h"
#include "IGUIElement.h"

namespace PointCloudEngine
{
	class GUITab : public IGUIElement
	{
	public:
		HWND hwndTab = NULL;
		int* value = NULL;

		GUITab(HWND hwndParent, XMUINT2 pos, XMUINT2 size, std::initializer_list<std::wstring> entries, int* value)
		{
			// Create the tab menu
			int counter = 0;
			this->value = value;
			hwndTab = CreateWindowEx(NULL, WC_TABCONTROLW, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, pos.x, pos.y, size.x, size.y, hwndParent, NULL, NULL, NULL);

			TCITEM tcitem;
			tcitem.mask = TCIF_TEXT;

			// Add items to the tab
			for (auto it = entries.begin(); it != entries.end(); it++)
			{
				tcitem.pszText = (wchar_t*)it->c_str();
				TabCtrl_InsertItem(hwndTab, counter++, &tcitem);
			}

			TabCtrl_SetCurSel(hwndTab, *value);
		}

		void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (msg == WM_NOTIFY)
			{
				LPNMHDR info = (LPNMHDR)lParam;

				if ((info->hwndFrom == hwndTab) && (info->code == TCN_SELCHANGE))
				{
					// Invoke the passed function
					*value = TabCtrl_GetCurSel(hwndTab);
				}
			}
		}

		void Show(int SW_COMMAND)
		{
			ShowWindow(hwndTab, SW_COMMAND);
		}
	};
}
#endif