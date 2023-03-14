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
		std::function<void(int)> OnSelect = NULL;

		GUITab(HWND hwndParent, int positionX, int positionY, int width, int height, std::initializer_list<std::wstring> entries, std::function<void(int)> OnSelect)
		{
			// Create the tab menu
			int counter = 0;
			this->OnSelect = OnSelect;
			hwndElement = CreateWindowEx(NULL, WC_TABCONTROLW, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, positionX, positionY, width, height, hwndParent, NULL, NULL, NULL);
			IGUIElement::SetCustomWindowFontStyle(hwndElement);

			TCITEM tcitem;
			tcitem.mask = TCIF_TEXT;

			// Add items to the tab
			for (auto it = entries.begin(); it != entries.end(); it++)
			{
				tcitem.pszText = (wchar_t*)it->c_str();
				TabCtrl_InsertItem(hwndElement, counter++, &tcitem);
			}
		}

		void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (msg == WM_NOTIFY)
			{
				LPNMHDR info = (LPNMHDR)lParam;

				if ((info->hwndFrom == hwndElement) && (info->code == TCN_SELCHANGE))
				{
					// Invoke the passed function
					OnSelect(TabCtrl_GetCurSel(hwndElement));
				}
			}
		}
	};
}
#endif