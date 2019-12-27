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
		static UINT fps;
		static UINT vertexCount;

		static void Initialize();
		static void Release();
		static void Update();
		static void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
		static void SetVisible(bool visible);

	private:
		static bool initialized;
		static Vector2 guiSize;

		static HWND hwndGUI;
		static GUITab* tab;

		// General
		static std::vector<IGUIElement*> generalElements;
		static std::vector<IGUIElement*> splatsElements;
		static std::vector<IGUIElement*> sparseSplatsElements;
		static std::vector<IGUIElement*> pointsElements;
		static std::vector<IGUIElement*> sparsePointsElements;
		static std::vector<IGUIElement*> neuralNetworkElements;

		// Advanced
		static std::vector<IGUIElement*> advancedElements;

		static void ShowElements(std::vector<IGUIElement*> elements, int SW_COMMAND = SW_SHOW);
		static void DeleteElements(std::vector<IGUIElement*> elements);
		static void UpdateElements(std::vector<IGUIElement*> elements);
		static void HandleMessageElements(std::vector<IGUIElement*> elements, UINT msg, WPARAM wParam, LPARAM lParam);
		static void CreateContentGeneral();
		static void CreateContentAdvanced();

		static void OnSelectViewMode(int selection);
		static void OnSelectTab(int selection);
		static void OnClickLighting(bool checked);
		static void OnClickBlending(bool checked);

		static void ButtonOnClick();
		static void CheckboxOnClick(bool checked);
	};
}
#endif

