#include "PrecompiledHeader.h"
#include "GUI.h"

UINT GUI::fps = 0;
UINT GUI::vertexCount = 0;
bool GUI::initialized = false;
Vector2 GUI::guiSize = Vector2(400, 800);

HWND GUI::hwndGUI = NULL;
GUITab* GUI::tab = NULL;

// General
std::vector<IGUIElement*> GUI::generalElements;
std::vector<IGUIElement*> GUI::splatElements;
std::vector<IGUIElement*> GUI::octreeElements;
std::vector<IGUIElement*> GUI::sparseElements;
std::vector<IGUIElement*> GUI::neuralNetworkElements;

// Advanced
std::vector<IGUIElement*> GUI::advancedElements;

void PointCloudEngine::GUI::Initialize()
{
	if (!initialized)
	{
		// Place the gui window for changing settings next to the main window
		RECT rect;
		GetWindowRect(hwnd, &rect);

		// Initialize common controls library to use buttons and so on
		InitCommonControls();

		// This is the gui window
		hwndGUI = CreateWindowEx(NULL, L"PointCloudEngine", L"Settings", WS_SYSMENU | WS_CAPTION | WS_VISIBLE, rect.right, rect.top, guiSize.x, guiSize.y, hwnd, NULL, NULL, NULL);

		// Tab inside the gui window for choosing different groups of settings
		tab = new GUITab(hwndGUI, XMUINT2(0, 0), XMUINT2(guiSize.x, guiSize.y), { L"General", L"Advanced" }, OnSelectTab);

		// Create and show content for each tab
		CreateContentGeneral();
		CreateContentAdvanced();

		initialized = true;
	}

	OnSelectTab(0);
	OnSelectViewMode();
}

void PointCloudEngine::GUI::Release()
{
	initialized = false;

	// SafeDelete all GUI elements, HWNDs are released automatically
	DeleteElements(generalElements);
	DeleteElements(splatElements);
	DeleteElements(octreeElements);
	DeleteElements(sparseElements);
	DeleteElements(neuralNetworkElements);
	DeleteElements(advancedElements);
}

void PointCloudEngine::GUI::Update()
{
	if (!initialized)
		return;

	UpdateElements(generalElements);
	UpdateElements(splatElements);
	UpdateElements(octreeElements);
	UpdateElements(sparseElements);
	UpdateElements(neuralNetworkElements);
	UpdateElements(advancedElements);
}

void PointCloudEngine::GUI::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (!initialized)
		return;

	// WindowsProcess message will be forwarded to this function
	// The GUI is only interested in these kinds of messages for e.g. button notifications
	if (msg == WM_COMMAND || msg == WM_NOTIFY || msg == WM_HSCROLL)
	{
		tab->HandleMessage(msg, wParam, lParam);
		HandleMessageElements(generalElements, msg, wParam, lParam);
		HandleMessageElements(splatElements, msg, wParam, lParam);
		HandleMessageElements(octreeElements, msg, wParam, lParam);
		HandleMessageElements(sparseElements, msg, wParam, lParam);
		HandleMessageElements(neuralNetworkElements, msg, wParam, lParam);
		HandleMessageElements(advancedElements, msg, wParam, lParam);
	}
}

void PointCloudEngine::GUI::SetVisible(bool visible)
{
	ShowWindow(hwndGUI, visible ? SW_SHOW : SW_HIDE);
}

void PointCloudEngine::GUI::ShowElements(std::vector<IGUIElement*> elements, int SW_COMMAND)
{
	for (auto it = elements.begin(); it != elements.end(); it++)
	{
		(*it)->Show(SW_COMMAND);
	}
}

void PointCloudEngine::GUI::DeleteElements(std::vector<IGUIElement*> elements)
{
	for (auto it = elements.begin(); it != elements.end(); it++)
	{
		SafeDelete(*it);
	}

	elements.clear();
}

void PointCloudEngine::GUI::UpdateElements(std::vector<IGUIElement*> elements)
{
	for (auto it = elements.begin(); it != elements.end(); it++)
	{
		(*it)->Update();
	}
}

void PointCloudEngine::GUI::HandleMessageElements(std::vector<IGUIElement*> elements, UINT msg, WPARAM wParam, LPARAM lParam)
{
	for (auto it = elements.begin(); it != elements.end(); it++)
	{
		(*it)->HandleMessage(msg, wParam, lParam);
	}
}

void PointCloudEngine::GUI::CreateContentGeneral()
{
	generalElements.push_back(new GUIText(hwndGUI, { 10, 40 }, { 100, 20 }, L"View Mode "));
	generalElements.push_back(new GUIDropdown(hwndGUI, { 160, 35 }, { 130, 200 }, { L"Splats", L"Sparse Splats", L"Points", L"Sparse Points", L"Neural Network" }, OnSelectViewMode, &settings->viewMode));
	generalElements.push_back(new GUIDropdown(hwndGUI, { 160, 35 }, { 130, 200 }, { L"Splats", L"Octree Nodes", L"Normal Clusters" }, OnSelectViewMode, &settings->viewMode));
	generalElements.push_back(new GUIText(hwndGUI, { 10, 70 }, { 150, 20 }, L"Vertex Count "));
	generalElements.push_back(new GUIValue<UINT>(hwndGUI, { 160, 70 }, { 200, 20 }, &GUI::vertexCount));
	generalElements.push_back(new GUIText(hwndGUI, { 10, 100 }, { 150, 20 }, L"Frames per second "));
	generalElements.push_back(new GUIValue<UINT>(hwndGUI, { 160, 100 }, { 50, 20 }, &GUI::fps));
	generalElements.push_back(new GUIText(hwndGUI, { 10, 130 }, { 150, 20 }, L"Lighting "));
	generalElements.push_back(new GUICheckbox(hwndGUI, { 160, 130 }, { 20, 20 }, L"", NULL, &settings->useLighting));

	splatElements.push_back(new GUIText(hwndGUI, { 10, 160 }, { 150, 20 }, L"Blending "));
	splatElements.push_back(new GUICheckbox(hwndGUI, { 160, 160 }, { 20, 20 }, L"", NULL, &settings->useBlending));
	splatElements.push_back(new GUISlider<float>(hwndGUI, { 160, 190 }, { 130, 20 }, { 0, 100 }, 10, 0, L"Blend Factor", &settings->blendFactor));
	splatElements.push_back(new GUISlider<float>(hwndGUI, { 160, 220 }, { 130, 20 }, { 0, 1000 }, 1000, 0, L"Sampling Rate", &settings->samplingRate));

	octreeElements.push_back(new GUISlider<float>(hwndGUI, { 160, 250 }, { 130, 20 }, { 4, 100 }, settings->resolutionY * 4, 0, L"Splat Resolution", &settings->splatResolution));
	octreeElements.push_back(new GUISlider<float>(hwndGUI, { 160, 280 }, { 130, 20 }, { 0, 500 }, 100, -100, L"Overlap Factor", &settings->overlapFactor));
	octreeElements.push_back(new GUISlider<int>(hwndGUI, { 160, 310 }, { 130, 20 }, { 0, 1 + (UINT)settings->maxOctreeDepth }, 1, 1, L"Octree Level", &settings->octreeLevel));
	octreeElements.push_back(new GUIText(hwndGUI, { 10, 340 }, { 150, 20 }, L"Culling "));
	octreeElements.push_back(new GUICheckbox(hwndGUI, { 160, 340 }, { 20, 20 }, L"", NULL, &settings->useCulling));
	octreeElements.push_back(new GUIText(hwndGUI, { 10, 370 }, { 150, 20 }, L"GPU Traversal "));
	octreeElements.push_back(new GUICheckbox(hwndGUI, { 160, 370 }, { 20, 20 }, L"", NULL, &settings->useGPUTraversal));

	sparseElements.push_back(new GUISlider<float>(hwndGUI, { 160, 220 }, { 130, 20 }, { 0, 1000 }, 100, 0, L"Sparse Sampling Rate", &settings->sparseSamplingRate));
	sparseElements.push_back(new GUISlider<float>(hwndGUI, { 160, 250 }, { 130, 20 }, { 0, 1000 }, 1000, 0, L"Density", &settings->density));
}

void PointCloudEngine::GUI::CreateContentAdvanced()
{
	// TODO
}

void PointCloudEngine::GUI::OnSelectViewMode()
{
	ShowElements(octreeElements, SW_HIDE);
	ShowElements(splatElements, SW_HIDE);
	ShowElements(sparseElements, SW_HIDE);
	ShowElements(neuralNetworkElements, SW_HIDE);

	if (settings->useOctree)
	{
		ShowElements(splatElements);
		ShowElements(octreeElements);
	}
	else
	{
		switch (settings->viewMode)
		{
			case 0:
			{
				ShowElements(splatElements);
				break;
			}
			case 1:
			{
				ShowElements(splatElements);
				ShowElements(sparseElements);
				splatElements[3]->Show(SW_HIDE);
				sparseElements[1]->SetPosition({ 160, 250 });
				break;
			}
			case 3:
			{
				sparseElements[1]->Show(SW_SHOW);
				sparseElements[1]->SetPosition({ 160, 160 });
				break;
			}
			case 4:
			{
				ShowElements(neuralNetworkElements);
				break;
			}
		}
	}
}

void PointCloudEngine::GUI::OnSelectTab(int selection)
{
	ShowElements(generalElements, SW_HIDE);
	ShowElements(splatElements, SW_HIDE);
	ShowElements(octreeElements, SW_HIDE);
	ShowElements(sparseElements, SW_HIDE);
	ShowElements(neuralNetworkElements, SW_HIDE);
	ShowElements(advancedElements, SW_HIDE);

	switch (selection)
	{
		case 0:
		{
			ShowElements(generalElements);
			OnSelectViewMode();

			if (settings->useOctree)
			{
				generalElements[1]->Show(SW_HIDE);
			}
			else
			{
				generalElements[2]->Show(SW_HIDE);
			}
			break;
		}
		case 1:
		{
			ShowElements(advancedElements);
			break;
		}
	}
}
