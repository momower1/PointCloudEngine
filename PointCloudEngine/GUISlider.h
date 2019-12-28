#ifndef GUISLIDER_H
#define GUISLIDER_H

#pragma once
#include "PointCloudEngine.h"
#include "IGUIElement.h"

namespace PointCloudEngine
{
	template <typename T> class GUISlider : public IGUIElement
	{
	public:
		XMUINT2 size;
		HWND hwndSlider = NULL;
		HWND hwndSliderName = NULL;
		HWND hwndSliderValue = NULL;
		std::wstring name;
		T* value = NULL;
		float scale = 1;
		float offset = 0;

		GUISlider(HWND hwndParent, XMUINT2 pos, XMUINT2 size, XMUINT2 range, float scale, float offset, std::wstring name, T* value)
		{
			// The internal slider position is value * scale + offset
			this->size = size;
			this->name = name;
			this->value = value;
			this->scale = scale;
			this->offset = offset;
			hwndSlider = CreateWindowEx(NULL, TRACKBAR_CLASS, L"", TBS_NOTICKS | WS_CHILD | WS_VISIBLE, pos.x, pos.y, size.x, size.y, hwndParent, NULL, NULL, NULL);
			
			// Left and right buddy showing text with name and value of the slider
			hwndSliderName = CreateWindowEx(0, L"STATIC", name.c_str(), SS_LEFT | WS_CHILD | WS_VISIBLE, 0, 0, 148, size.y, hwndParent, NULL, NULL, NULL);
			hwndSliderValue = CreateWindowEx(0, L"STATIC", std::to_wstring(*value).c_str(), SS_LEFT | WS_CHILD | WS_VISIBLE, 0, 0, 43, size.y, hwndParent, NULL, NULL, NULL);
			SendMessage(hwndSlider, TBM_SETBUDDY, (WPARAM)TRUE, (LPARAM)hwndSliderName);
			SendMessage(hwndSlider, TBM_SETBUDDY, (WPARAM)FALSE, (LPARAM)hwndSliderValue);

			// Set range of the slider
			UINT sliderPosition = (*value * scale) + offset;
			SendMessage(hwndSlider, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(range.x, range.y));
			SendMessage(hwndSlider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)sliderPosition);
		}

		void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (msg == WM_HSCROLL)
			{
				if ((HWND)lParam == hwndSlider)
				{
					// Overwrite the value and display it next to the slider
					*value = (SendMessage(hwndSlider, TBM_GETPOS, 0, 0) - offset) / scale;
					SetWindowText(hwndSliderValue, std::to_wstring(*value).c_str());
				}
			}
		}

		void SetPosition(XMUINT2 position)
		{
			MoveWindow(hwndSlider, position.x, position.y, size.x, size.y, true);
			SendMessage(hwndSlider, TBM_SETBUDDY, (WPARAM)TRUE, (LPARAM)hwndSliderName);
			SendMessage(hwndSlider, TBM_SETBUDDY, (WPARAM)FALSE, (LPARAM)hwndSliderValue);
		}

		void Show(int SW_COMMAND)
		{
			ShowWindow(hwndSlider, SW_COMMAND);
			ShowWindow(hwndSliderName, SW_COMMAND);
			ShowWindow(hwndSliderValue, SW_COMMAND);
		}
	};
}
#endif