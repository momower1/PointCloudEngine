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
		HWND hwndGeneral = NULL;
		HWND hwndAdvanced = NULL;
		HWND hwndDropdown = NULL;
		HWND hwndSlider = NULL;

		HDC hdcGeneral = NULL;
		HDC hdcAdvanced = NULL;

		void CreateContentGeneral();
		void CreateContentAdvanced();
	};
}
#endif

