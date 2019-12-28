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
		static UINT cameraSelection;
		static bool waypointPreview;
		static float waypointPreviewLocation;
		static WaypointRenderer* waypointRenderer;

		static void Initialize();
		static void Release();
		static void Update();
		static void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
		static void SetVisible(bool visible);

	private:
		static bool initialized;
		static Vector3 waypointStartPosition;
		static Matrix waypointStartRotation;
		static Vector2 guiSize;

		static HWND hwndGUI;
		static GUITab* tab;

		// General
		static std::vector<IGUIElement*> generalElements;
		static std::vector<IGUIElement*> splatElements;
		static std::vector<IGUIElement*> octreeElements;
		static std::vector<IGUIElement*> sparseElements;
		static std::vector<IGUIElement*> neuralNetworkElements;

		// Advanced
		static std::vector<IGUIElement*> advancedElements;

		static void ShowElements(std::vector<IGUIElement*> elements, int SW_COMMAND = SW_SHOW);
		static void DeleteElements(std::vector<IGUIElement*> elements);
		static void UpdateElements(std::vector<IGUIElement*> elements);
		static void HandleMessageElements(std::vector<IGUIElement*> elements, UINT msg, WPARAM wParam, LPARAM lParam);
		static void CreateContentGeneral();
		static void CreateContentAdvanced();

		static void OnSelectViewMode();
		static void OnSelectTab(int selection);
		static void OnWaypointAdd();
		static void OnWaypointRemove();
		static void OnWaypointToggle();
		static void OnWaypointPreview();
	};
}
#endif
