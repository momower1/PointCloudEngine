#ifndef GUITEXT_H
#define GUITEXT_H

#pragma once
#include "PointCloudEngine.h"
#include "IGUIElement.h"

namespace PointCloudEngine
{
	class GUIText : public IGUIElement
	{
	public:
		GUIText(HWND hwndParent, int positionX, int positionY, int width, int height, std::wstring text)
		{
			// Window that just displays some text
			hwndElement = CreateWindowEx(0, WC_STATIC, text.c_str(), SS_LEFT | WS_CHILD | WS_VISIBLE, positionX, positionY, width, height, hwndParent, NULL, NULL, NULL);
			IGUIElement::SetCustomWindowFontStyle(hwndElement);
		}

		void SetText(std::wstring text)
		{
			SetWindowText(hwndElement, text.c_str());
		}
	};
}
#endif