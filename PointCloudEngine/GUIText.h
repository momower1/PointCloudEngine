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
		HWND hwndText = NULL;
		std::wstring text;

		GUIText(HWND hwndParent, XMUINT2 pos, XMUINT2 size, std::wstring text)
		{
			// Window that just displays some text
			this->text = text;
			hwndText = CreateWindowEx(0, L"STATIC", text.c_str(), SS_LEFT | WS_CHILD | WS_VISIBLE, pos.x, pos.y, size.x, size.y, hwndParent, NULL, NULL, NULL);
		}

		void SetText(std::wstring text)
		{
			SetWindowText(hwndText, text.c_str());
		}

		void Show(int SW_COMMAND)
		{
			ShowWindow(hwndText, SW_COMMAND);
		}
	};
}
#endif