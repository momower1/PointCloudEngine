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
		static UINT triangleCount, uvCount, normalCount, submeshCount, textureCount;
		static UINT waypointCount;

		static void Initialize();
		static void Release();
		static void Update();
		static void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
		static void SetVisible(bool visible);

	private:
		static bool initialized;
		static int viewModeSelection;
		static int shadingModeSelection;
		static int resolutionX;
		static int resolutionY;

		static GUITab* tabGroundTruth;
		static GUITab* tabOctree;

		// General
		static std::vector<IGUIElement*> rendererElements;
		static std::vector<IGUIElement*> splatElements;
		static std::vector<IGUIElement*> octreeElements;
		static std::vector<IGUIElement*> sparseElements;
		static std::vector<IGUIElement*> pointElements;
		static std::vector<IGUIElement*> pullPushElements;
		static std::vector<IGUIElement*> meshElements;
		static std::vector<IGUIElement*> neuralNetworkElements;

		// Advanced
		static std::vector<IGUIElement*> advancedElements;

		// Dataset
		static std::vector<IGUIElement*> datasetElements;

		static void ShowElements(std::vector<IGUIElement*> elements, int SW_COMMAND = SW_SHOW);
		static void DeleteElements(std::vector<IGUIElement*> elements);
		static void UpdateElements(std::vector<IGUIElement*> elements);
		static void HandleMessageElements(std::vector<IGUIElement*> elements, UINT msg, WPARAM wParam, LPARAM lParam);
		static void CreateRendererElements();
		static void CreateAdvancedElements();
		static void CreateDatasetElements();

		static void OnSelectViewMode();
		static void OnSelectTab(int selection);
		static void OnSelectShadingMode();
		static void OnChangeResolution();
		static void OnApplyResolution();
		static void OnLoadMeshFromOBJFile();
		static void OnWaypointAdd();
		static void OnWaypointRemove();
		static void OnWaypointToggle();
		static void OnWaypointPreview();
		static void OnGenerateWaypointDataset();
		static void OnGenerateSphereDataset();
	};
}
#endif

