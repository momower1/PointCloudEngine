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
std::vector<IGUIElement*> GUI::splatsElements;
std::vector<IGUIElement*> GUI::sparseSplatsElements;
std::vector<IGUIElement*> GUI::pointsElements;
std::vector<IGUIElement*> GUI::sparsePointsElements;
std::vector<IGUIElement*> GUI::neuralNetworkElements;

// Advanced
std::vector<IGUIElement*> GUI::advancedElements;

void PointCloudEngine::GUI::Initialize()
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
	OnSelectTab(0);
	OnSelectViewMode(settings->viewMode);

	initialized = true;
}

void PointCloudEngine::GUI::Release()
{
	initialized = false;

	// SafeDelete all GUI elements, HWNDs are released automatically
	DeleteElements(generalElements);
	DeleteElements(splatsElements);
	DeleteElements(sparseSplatsElements);
	DeleteElements(pointsElements);
	DeleteElements(sparsePointsElements);
	DeleteElements(neuralNetworkElements);
	DeleteElements(advancedElements);
}

void PointCloudEngine::GUI::Update()
{
	if (!initialized)
		return;

	UpdateElements(generalElements);
	UpdateElements(splatsElements);
	UpdateElements(sparseSplatsElements);
	UpdateElements(pointsElements);
	UpdateElements(sparsePointsElements);
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
		HandleMessageElements(splatsElements, msg, wParam, lParam);
		HandleMessageElements(sparseSplatsElements, msg, wParam, lParam);
		HandleMessageElements(pointsElements, msg, wParam, lParam);
		HandleMessageElements(sparsePointsElements, msg, wParam, lParam);
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
	generalElements.push_back(new GUIDropdown(hwndGUI, { 160, 35 }, { 130, 200 }, { L"Splats", L"Sparse Splats", L"Points", L"Sparse Points", L"Neural Network" }, OnSelectViewMode, settings->viewMode));
	generalElements.push_back(new GUIText(hwndGUI, { 10, 70 }, { 150, 20 }, L"Vertex Count "));
	generalElements.push_back(new GUIValue<UINT>(hwndGUI, { 160, 70 }, { 200, 20 }, &GUI::vertexCount));
	generalElements.push_back(new GUIText(hwndGUI, { 10, 100 }, { 150, 20 }, L"Frames per second "));
	generalElements.push_back(new GUIValue<UINT>(hwndGUI, { 160, 100 }, { 50, 20 }, &GUI::fps));
	generalElements.push_back(new GUIText(hwndGUI, { 10, 130 }, { 150, 20 }, L"Lighting "));
	generalElements.push_back(new GUICheckbox(hwndGUI, { 160, 130 }, { 20, 20 }, L"", OnClickLighting, settings->useLighting));

	splatsElements.push_back(new GUIText(hwndGUI, { 10, 160 }, { 150, 20 }, L"Blending "));
	splatsElements.push_back(new GUICheckbox(hwndGUI, { 160, 160 }, { 20, 20 }, L"", OnClickBlending, settings->useBlending));
	splatsElements.push_back(new GUISlider<float>(hwndGUI, { 160, 190 }, { 130, 20 }, { 0, 100 }, 10, 0, L"Blend Factor", &settings->blendFactor));
	splatsElements.push_back(new GUISlider<float>(hwndGUI, { 160, 220 }, { 130, 20 }, { 0, 1000 }, 1000, 0, L"Sampling Rate", &settings->samplingRate));

	sparseSplatsElements.push_back(new GUISlider<float>(hwndGUI, { 160, 220 }, { 130, 20 }, { 0, 1000 }, 100, 0, L"Sparse Sampling Rate", &settings->sparseSamplingRate));
	sparseSplatsElements.push_back(new GUISlider<float>(hwndGUI, { 160, 250 }, { 130, 20 }, { 0, 1000 }, 1000, 0, L"Point Density", &settings->density));
}

void PointCloudEngine::GUI::CreateContentAdvanced()
{
	advancedElements.push_back(new GUIButton(hwndGUI, { 100, 300 }, { 100, 30 }, L"Press me", GUI::ButtonOnClick));
	advancedElements.push_back(new GUICheckbox(hwndGUI, { 100, 500 }, { 100, 30 }, L"Check me out", CheckboxOnClick));
}

void PointCloudEngine::GUI::OnSelectViewMode(int selection)
{
	settings->viewMode = selection;
	ShowElements(splatsElements, SW_HIDE);
	ShowElements(sparseSplatsElements, SW_HIDE);
	ShowElements(pointsElements, SW_HIDE);
	ShowElements(sparsePointsElements, SW_HIDE);
	ShowElements(neuralNetworkElements, SW_HIDE);

	switch (selection)
	{
		case 0:
		{
			ShowElements(splatsElements);
			break;
		}
		case 1:
		{
			ShowElements(splatsElements);
			ShowElements(sparseSplatsElements);
			splatsElements[3]->Show(SW_HIDE);
			break;
		}
		case 2:
		{
			ShowElements(pointsElements);
			break;
		}
		case 3:
		{
			ShowElements(sparsePointsElements);
			break;
		}
		case 4:
		{
			ShowElements(neuralNetworkElements);
			break;
		}
	}
}

void PointCloudEngine::GUI::OnSelectTab(int selection)
{
	ShowElements(generalElements, SW_HIDE);
	ShowElements(splatsElements, SW_HIDE);
	ShowElements(sparseSplatsElements, SW_HIDE);
	ShowElements(pointsElements, SW_HIDE);
	ShowElements(sparsePointsElements, SW_HIDE);
	ShowElements(neuralNetworkElements, SW_HIDE);
	ShowElements(advancedElements, SW_HIDE);

	switch (selection)
	{
		case 0:
		{
			ShowElements(generalElements);
			OnSelectViewMode(settings->viewMode);
			break;
		}
		case 1:
		{
			ShowElements(advancedElements);
			break;
		}
	}
}

void PointCloudEngine::GUI::OnClickLighting(bool checked)
{
	settings->useLighting = checked;
}

void PointCloudEngine::GUI::OnClickBlending(bool checked)
{
	settings->useBlending = checked;
}

void PointCloudEngine::GUI::ButtonOnClick()
{
	OutputDebugString(L"You pressed me, well done!\n");
}

void PointCloudEngine::GUI::CheckboxOnClick(bool checked)
{
	OutputDebugString(checked ? L"Checked\n" : L"Unchecked!\n");
}
