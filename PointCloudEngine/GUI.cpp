#include "PrecompiledHeader.h"
#include "GUI.h"

bool GUI::initialized = false;
Vector2 GUI::guiSize = Vector2(400, 800);

HWND GUI::hwndGUI = NULL;
GUITab* GUI::tab = NULL;

// General
std::vector<IGUIElement*> GUI::generalElements;

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
	tab = new GUITab(hwndGUI, XMUINT2(0, 0), XMUINT2(guiSize.x, guiSize.y), { L"General", L"Advanced" }, TabOnSelect);

	// Create and show content for each tab
	CreateContentGeneral();
	CreateContentAdvanced();
	ShowElements(advancedElements, SW_HIDE);

	initialized = true;
}

void PointCloudEngine::GUI::Release()
{
	initialized = false;

	// SafeDelete all GUI elements, HWNDs are released automatically
	DeleteElements(generalElements);
	DeleteElements(advancedElements);
}

void PointCloudEngine::GUI::Update()
{
	if (!initialized)
		return;

	UpdateElements(generalElements);
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
		HandleMessageElements(advancedElements, msg, wParam, lParam);
	}
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
	generalElements.push_back(new GUIText(hwndGUI, { 0, 50 }, { 100, 20 }, L"View Mode: "));
	generalElements.push_back(new GUIValue<float>(hwndGUI, { 300, 50 }, { 50, 20 }, &settings->scale));
	generalElements.push_back(new GUIDropdown(hwndGUI, { 100, 50 }, { 200, 200 }, { L"Splats", L"Sparse Splats", L"Points", L"Sparse Points", L"Neural Network" }, &settings->viewMode));
	generalElements.push_back(new GUISlider<float>(hwndGUI, { 100, 100 }, { 200, 20 }, { 0, 1000 }, 1000, 0, L"Point Density", &settings->density));
	generalElements.push_back(new GUISlider<float>(hwndGUI, { 100, 150 }, { 200, 20 }, { 0, 100 }, 10, 0, L"Blend Factor", &settings->blendFactor));
}

void PointCloudEngine::GUI::CreateContentAdvanced()
{
	advancedElements.push_back(new GUIButton(hwndGUI, { 100, 300 }, { 100, 30 }, L"Press me", GUI::ButtonOnClick));
	advancedElements.push_back(new GUICheckbox(hwndGUI, { 100, 500 }, { 100, 30 }, L"Check me out", CheckboxOnClick));
}

void PointCloudEngine::GUI::TabOnSelect(int selection)
{
	if (selection == 0)
	{
		ShowElements(generalElements, SW_SHOW);
		ShowElements(advancedElements, SW_HIDE);
	}
	else
	{
		ShowElements(generalElements, SW_HIDE);
		ShowElements(advancedElements, SW_SHOW);
	}
}

void PointCloudEngine::GUI::ButtonOnClick()
{
	OutputDebugString(L"You pressed me, well done!\n");
}

void PointCloudEngine::GUI::CheckboxOnClick(bool checked)
{
	OutputDebugString(checked ? L"Checked\n" : L"Unchecked!\n");
}
