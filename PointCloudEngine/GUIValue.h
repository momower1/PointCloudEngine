#ifndef GUIVALUE_H
#define GUIVALUE_H

#pragma once
#include "PointCloudEngine.h"
#include "IGUIElement.h"

namespace PointCloudEngine
{
	template <typename T> class GUIValue : public IGUIElement
	{
	public:
		HWND hwndValue = NULL;
		T* value = NULL;
		T oldValue;

		GUIValue(HWND hwndParent, XMUINT2 pos, XMUINT2 size, T* value)
		{
			// Displays the value as a string
			oldValue = *value;
			this->value = value;
			hwndValue = CreateWindowEx(0, L"STATIC", std::to_wstring(*value).c_str(), SS_LEFT | WS_CHILD | WS_VISIBLE, pos.x, pos.y, size.x, size.y, hwndParent, NULL, NULL, NULL);
		}

		void Update()
		{
			// Only update text if the value changed
			if (*value != oldValue)
			{
				oldValue = *value;
				SetWindowText(hwndValue, std::to_wstring(*value).c_str());
			}
		}

		void Show(int SW_COMMAND)
		{
			ShowWindow(hwndValue, SW_COMMAND);
		}
	};
}
#endif