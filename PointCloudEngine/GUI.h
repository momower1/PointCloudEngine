#ifndef GUI_H
#define GUI_H

#pragma once
#include "PointCloudEngine.h"
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
		GUI();
		~GUI();
		void Update();
		void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		Vector2 guiSize = Vector2(400, 800);

		HWND hwndGUI = NULL;
		GUITab* guiTab = NULL;
		int tabSelection = 0;

		// General
		GUIText* textDropdown = NULL;
		GUIValue<float>* scaleValue = NULL;
		GUIDropdown* dropdown = NULL;
		GUISlider<float>* densitySlider = NULL;
		GUISlider<float>* blendFactorSlider = NULL;

		// Advanced
		HWND hwndButton = NULL;

		void CreateContentGeneral();
		void CreateContentAdvanced();
		void ShowContentGeneral();
		void ShowContentAdvanced();
	};
}
#endif

