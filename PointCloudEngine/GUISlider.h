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
		HWND hwndSliderName = NULL;
		HWND hwndSliderValue = NULL;
		std::function<void()> OnChange = NULL;
		std::wstring name;
		T* value = NULL;
		float scale = 1;
		float offset = 0;
		UINT precision = 6;

		GUISlider(HWND hwndParent, int positionX, int positionY, int width, int height, int rangeMin, int rangeMax, float scale, float offset, std::wstring name, T* value, UINT precision = 1, UINT nameWidth = 148, UINT valueWidth = 40, std::function<void()> OnChange = NULL)
		{
			// The internal slider position is value * scale + offset
			this->name = name;
			this->value = value;
			this->scale = scale;
			this->offset = offset;
			this->precision = precision;
			this->OnChange = OnChange;
			hwndElement = CreateWindowEx(NULL, TRACKBAR_CLASS, L"", TBS_NOTICKS | WS_CHILD | WS_VISIBLE, positionX, positionY, width, height, hwndParent, NULL, NULL, NULL);
			
			// Left and right buddy showing text with name and value of the slider
			hwndSliderName = CreateWindowEx(0, WC_STATIC, name.c_str(), SS_LEFT | WS_CHILD | WS_VISIBLE, 0, 0, nameWidth, height, hwndParent, NULL, NULL, NULL);
			hwndSliderValue = CreateWindowEx(0, WC_STATIC, ToPrecisionWstring(*value, precision).c_str(), SS_LEFT | WS_CHILD | WS_VISIBLE, 0, 0, valueWidth, height, hwndParent, NULL, NULL, NULL);
			SendMessage(hwndElement, TBM_SETBUDDY, (WPARAM)TRUE, (LPARAM)hwndSliderName);
			SendMessage(hwndElement, TBM_SETBUDDY, (WPARAM)FALSE, (LPARAM)hwndSliderValue);
			IGUIElement::SetCustomWindowFontStyle(hwndSliderName);
			IGUIElement::SetCustomWindowFontStyle(hwndSliderValue);

			// Set range of the slider
			UINT sliderPosition = (*value * scale) + offset;
			SendMessage(hwndElement, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(rangeMin, rangeMax));
			SendMessage(hwndElement, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)sliderPosition);
		}

		void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (msg == WM_HSCROLL)
			{
				if ((HWND)lParam == hwndElement)
				{
					// Overwrite the value and display it next to the slider
					*value = (SendMessage(hwndElement, TBM_GETPOS, 0, 0) - offset) / scale;
					SetWindowText(hwndSliderValue, ToPrecisionWstring(*value, precision).c_str());

					// Invoke function on change
					if (OnChange != NULL)
					{
						OnChange();
					}
				}
			}
		}

		void SetPosition(int positionX, int positionY)
		{
			IGUIElement::SetPosition(positionX, positionY);
			SendMessage(hwndElement, TBM_SETBUDDY, (WPARAM)TRUE, (LPARAM)hwndSliderName);
			SendMessage(hwndElement, TBM_SETBUDDY, (WPARAM)FALSE, (LPARAM)hwndSliderValue);
		}

		void SetRange(UINT min, UINT max)
		{
			SendMessage(hwndElement, TBM_SETRANGEMIN, (WPARAM)TRUE, (LPARAM)min);
			SendMessage(hwndElement, TBM_SETRANGEMAX, (WPARAM)TRUE, (LPARAM)max);
		}

		void Show(int SW_COMMAND)
		{
			IGUIElement::Show(SW_COMMAND);
			ShowWindow(hwndSliderName, SW_COMMAND);
			ShowWindow(hwndSliderValue, SW_COMMAND);
		}
	};
}
#endif