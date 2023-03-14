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
		T* value = NULL;
		T oldValue;

		GUIValue(HWND hwndParent, int positionX, int positionY, int width, int height, T* value)
		{
			this->value = value;

			if (value != NULL)
			{
				// Displays the value as a string
				oldValue = *value;
				hwndElement = CreateWindowEx(0, WC_STATIC, std::to_wstring(*value).c_str(), SS_LEFT | WS_CHILD | WS_VISIBLE, positionX, positionY, width, height, hwndParent, NULL, NULL, NULL);
			}
			else
			{
				hwndElement = CreateWindowEx(0, WC_STATIC, L"", SS_LEFT | WS_CHILD | WS_VISIBLE, positionX, positionY, width, height, hwndParent, NULL, NULL, NULL);
			}

			IGUIElement::SetCustomWindowFontStyle(hwndElement);
		}

		void Update()
		{
			if (value == NULL)
			{
				SetWindowText(hwndElement, L"");
			}
			else if (*value != oldValue)
			{
				// Only update text if the value changed
				oldValue = *value;
				SetWindowText(hwndElement, std::to_wstring(*value).c_str());
			}
		}
	};
}
#endif