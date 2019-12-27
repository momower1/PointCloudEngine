#ifndef GUI_H
#define GUI_H

#pragma once
#include "PointCloudEngine.h"
#include "GUIButton.h"
#include "GUICheckbox.h"
#include "GUIDropdown.h"
#include "GUISlider.h"
#include "GUITab.h"
#include "GUIText.h"
#include "GUIValue.h"

namespace PointCloudEngine
{
	class GUI
	{
	public:
		static void Initialize();
		static void Release();
		static void Update();
		static void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		static bool initialized;
		static Vector2 guiSize;

		static HWND hwndGUI;
		static GUITab* guiTab;

		// General
		static GUIText* textDropdown;
		static GUIValue<float>* scaleValue;
		static GUIDropdown* dropdown;
		static GUISlider<float>* densitySlider;
		static GUISlider<float>* blendFactorSlider;

		// Advanced
		static GUIButton* button;
		static GUICheckbox* checkbox;

		static void CreateContentGeneral();
		static void CreateContentAdvanced();
		static void ShowContentGeneral();
		static void ShowContentAdvanced();

		static void TabOnSelect(int selection);
		static void ButtonOnClick();
		static void CheckboxOnClick(bool checked);
	};
}
#endif

