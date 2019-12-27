#include "PrecompiledHeader.h"
#include "GUI.h"

bool GUI::initialized = false;
Vector2 GUI::guiSize = Vector2(400, 800);

HWND GUI::hwndGUI = NULL;
GUITab* GUI::guiTab = NULL;

// General
GUIText* GUI::textDropdown = NULL;
GUIValue<float>* GUI::scaleValue = NULL;
GUIDropdown* GUI::dropdown = NULL;
GUISlider<float>* GUI::densitySlider = NULL;
GUISlider<float>* GUI::blendFactorSlider = NULL;

// Advanced
GUIButton* GUI::button = NULL;
GUICheckbox* GUI::checkbox = NULL;

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
	guiTab = new GUITab(hwndGUI, XMUINT2(0, 0), XMUINT2(guiSize.x, guiSize.y), { L"General", L"Advanced" }, TabOnSelect);

	CreateContentGeneral();
	CreateContentAdvanced();
	ShowContentGeneral();

	initialized = true;
}

void PointCloudEngine::GUI::Release()
{
	initialized = false;

	// TODO: SafeDelete all GUI elements
}

void PointCloudEngine::GUI::Update()
{
	if (!initialized)
		return;

	scaleValue->Update();
}

void PointCloudEngine::GUI::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (!initialized)
		return;

	switch (msg)
	{
		case WM_COMMAND:
		{
			if (dropdown != NULL)
				dropdown->HandleMessage(msg, wParam, lParam);

			if (button != NULL)
				button->HandleMessage(msg, wParam, lParam);
			
			if (checkbox != NULL)
				checkbox->HandleMessage(msg, wParam, lParam);
			break;
		}
		case WM_NOTIFY:
		{
			if (guiTab != NULL)
				guiTab->HandleMessage(msg, wParam, lParam);
			break;
		}
		case WM_HSCROLL:
		{
			if (densitySlider != NULL)
				densitySlider->HandleMessage(msg, wParam, lParam);
			if (blendFactorSlider != NULL)
				blendFactorSlider->HandleMessage(msg, wParam, lParam);
			break;
		}
	}

	//OutputDebugString((std::to_wstring(msg) + L" " + std::to_wstring(wParam) + L" " + std::to_wstring(lParam)).c_str());
}

void PointCloudEngine::GUI::CreateContentGeneral()
{
	textDropdown = new GUIText(hwndGUI, XMUINT2(0, 50), XMUINT2(100, 20), L"View Mode: ");
	scaleValue = new GUIValue<float>(hwndGUI, XMUINT2(300, 50), XMUINT2(50, 20), &settings->scale);

	dropdown = new GUIDropdown(hwndGUI, XMUINT2(100, 50), XMUINT2(200, 200), { L"Splats", L"Sparse Splats", L"Points", L"Sparse Points", L"Neural Network" }, &settings->viewMode);

	densitySlider = new GUISlider<float>(hwndGUI, XMUINT2(100, 100), XMUINT2(200, 20), XMUINT2(0, 1000), 1000, 0, L"Point Density", &settings->density);
	blendFactorSlider = new GUISlider<float>(hwndGUI, XMUINT2(100, 150), XMUINT2(200, 20), XMUINT2(0, 100), 10, 0, L"Blend Factor", &settings->blendFactor);
}

void PointCloudEngine::GUI::CreateContentAdvanced()
{
	button = new GUIButton(hwndGUI, { 100, 300 }, { 100, 30 }, L"Press me", GUI::ButtonOnClick);
	checkbox = new GUICheckbox(hwndGUI, { 100, 500 }, { 100, 30 }, L"Check me out", CheckboxOnClick);
}

void PointCloudEngine::GUI::ShowContentGeneral()
{
	dropdown->Show(SW_SHOW);
	densitySlider->Show(SW_SHOW);
	blendFactorSlider->Show(SW_SHOW);
	button->Show(SW_HIDE);
	checkbox->Show(SW_HIDE);
}

void PointCloudEngine::GUI::ShowContentAdvanced()
{
	dropdown->Show(SW_HIDE);
	densitySlider->Show(SW_HIDE);
	blendFactorSlider->Show(SW_HIDE);
	button->Show(SW_SHOW);
	checkbox->Show(SW_SHOW);
}

void PointCloudEngine::GUI::TabOnSelect(int selection)
{
	if (selection == 0)
	{
		ShowContentGeneral();
	}
	else
	{
		ShowContentAdvanced();
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
