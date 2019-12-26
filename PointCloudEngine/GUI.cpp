#include "PrecompiledHeader.h"
#include "GUI.h"

PointCloudEngine::GUI::GUI()
{
	// Place the gui window for changing settings next to the main window
	RECT rect;
	GetWindowRect(hwnd, &rect);

	// Initialize common controls library to use buttons and so on
	InitCommonControls();

	// This is the gui window
	hwndGUI = CreateWindowEx(NULL, L"PointCloudEngine", L"Settings", WS_SYSMENU | WS_CAPTION | WS_VISIBLE, rect.right, rect.top, guiSize.x, guiSize.y, hwnd, NULL, NULL, NULL);

	// Tab inside the gui window for choosing different groups of settings
	guiTab = new GUITab(hwndGUI, XMUINT2(0, 0), XMUINT2(guiSize.x, guiSize.y), { L"General", L"Advanced" }, &tabSelection);

	CreateContentGeneral();
	CreateContentAdvanced();
	ShowContentGeneral();
}

PointCloudEngine::GUI::~GUI()
{
	// TODO: SafeDelete all GUI elements
}

void PointCloudEngine::GUI::Update()
{
	scaleValue->Update();

	if (tabSelection == 0)
	{
		ShowContentGeneral();
	}
	else
	{
		ShowContentAdvanced();
	}
}

void PointCloudEngine::GUI::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	// During the creation of this instance there might be some messages arriving
	// Make sure to ignore all these messages to avoid a null reference
	if (this == NULL)
		return;

	switch (msg)
	{
		case WM_COMMAND:
		{
			if (dropdown != NULL)
				dropdown->HandleMessage(msg, wParam, lParam);

			if (HIWORD(wParam) == BN_CLICKED)
			{
				// Button handler
				OutputDebugString(L"You pressed a button!\n");
			}
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
	// For the advanced tab
	hwndButton = CreateWindowEx(NULL, L"BUTTON", L"Press me", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 100, 300, 100, 30, hwndGUI, NULL, NULL, NULL);
}

void PointCloudEngine::GUI::ShowContentGeneral()
{
	dropdown->Show(SW_SHOW);
	densitySlider->Show(SW_SHOW);
	blendFactorSlider->Show(SW_SHOW);
	ShowWindow(hwndButton, SW_HIDE);
}

void PointCloudEngine::GUI::ShowContentAdvanced()
{
	dropdown->Show(SW_HIDE);
	densitySlider->Show(SW_HIDE);
	blendFactorSlider->Show(SW_HIDE);
	ShowWindow(hwndButton, SW_SHOW);
}
