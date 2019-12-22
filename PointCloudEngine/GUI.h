#ifndef GUI_H
#define GUI_H

#pragma once
#include "PointCloudEngine.h"

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
		HWND hwndTab = NULL;

		// General
		HWND hwndDropdown = NULL;

		// Advanced
		HWND hwndButton = NULL;
		HWND hwndSlider = NULL;
		HWND hwndSliderName = NULL;
		HWND hwndSliderValue = NULL;

		HDC hdc = NULL;

		void CreateContentGeneral();
		void CreateContentAdvanced();
		void ShowContentGeneral();
		void ShowContentAdvanced();
	};
}
#endif

