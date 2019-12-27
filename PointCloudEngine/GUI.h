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
		static GUITab* tab;

		// General
		static std::vector<IGUIElement*> generalElements;

		// Advanced
		static std::vector<IGUIElement*> advancedElements;

		static void ShowElements(std::vector<IGUIElement*> elements, int SW_COMMAND);
		static void DeleteElements(std::vector<IGUIElement*> elements);
		static void UpdateElements(std::vector<IGUIElement*> elements);
		static void HandleMessageElements(std::vector<IGUIElement*> elements, UINT msg, WPARAM wParam, LPARAM lParam);
		static void CreateContentGeneral();
		static void CreateContentAdvanced();

		static void TabOnSelect(int selection);
		static void ButtonOnClick();
		static void CheckboxOnClick(bool checked);
	};
}
#endif

