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
	hwndTab = CreateWindowEx(NULL, WC_TABCONTROLW, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, 0, 0, guiSize.x, guiSize.y, hwndGUI, NULL, NULL, NULL);

	TCITEM tcitem;
	tcitem.mask = TCIF_TEXT;

	tcitem.pszText = L"General";
	TabCtrl_InsertItem(hwndTab, 0, &tcitem);

	tcitem.pszText = L"Advanced";
	TabCtrl_InsertItem(hwndTab, 1, &tcitem);

	CreateContentGeneral();
	CreateContentAdvanced();
	ShowContentGeneral();

	// Load a specific font for DrawText and TextOut functions
	HFONT segoe = CreateFont(24, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, TEXT("Segoe UI"));
	hdc = GetDC(hwndGUI);
	SelectObject(hdc, segoe);
}

PointCloudEngine::GUI::~GUI()
{
	ReleaseDC(hwndGUI, hdc);
}

void PointCloudEngine::GUI::Update()
{
	// Text on general tab
	TextOut(hdc, 10, 200, L"Sample with font!", 17);

	// Text on advanced tab
	RECT rect;
	GetClientRect(hwndGUI, &rect);
	DrawText(hdc, L"Hello World!", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void PointCloudEngine::GUI::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_COMMAND:
		{
			// Button handler
			if (((LPNMHDR)lParam)->code == BN_CLICKED)
			{
				OutputDebugString(L"You pressed a button!\n");
			}
			break;
		}
		case WM_NOTIFY:
		{
			// Tab handler
			if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)
			{
				if (TabCtrl_GetCurSel(hwndTab) == 0)
				{
					ShowContentGeneral();
				}
				else
				{
					ShowContentAdvanced();
				}
			}
			break;
		}
		case WM_HSCROLL:
		{
			// Slider
			if (LOWORD(wParam) == TB_ENDTRACK)
			{
				float sliderPosition = SendMessage(hwndSlider, TBM_GETPOS, 0, 0) / 100.0f;
				SetWindowText(hwndSliderValue, std::to_wstring(sliderPosition).c_str());
			}
			break;
		}
	}

	//OutputDebugString((std::to_wstring(msg) + L" " + std::to_wstring(wParam) + L" " + std::to_wstring(lParam)).c_str());
}

void PointCloudEngine::GUI::CreateContentGeneral()
{
	// For the general tab
	hwndDropdown = CreateWindowEx(NULL, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE, 25, 50, 100, 60, hwndGUI, NULL, NULL, NULL);

	// Add items to the dropdown
	SendMessage(hwndDropdown, CB_ADDSTRING, 0, (LPARAM)L"Eagle");
	SendMessage(hwndDropdown, CB_ADDSTRING, 0, (LPARAM)L"Hamster");
	SendMessage(hwndDropdown, CB_SETCURSEL, 0, 0);
}

void PointCloudEngine::GUI::CreateContentAdvanced()
{
	// For the advanced tab
	hwndButton = CreateWindowEx(NULL, L"BUTTON", L"Press me", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 100, 300, 100, 30, hwndGUI, NULL, NULL, NULL);

	// Slider
	hwndSlider = CreateWindowEx(NULL, TRACKBAR_CLASS, L"", TBS_NOTICKS | TBS_TOOLTIPS | WS_CHILD | WS_VISIBLE, 100, 100, 150, 20, hwndGUI, NULL, NULL, NULL);
	hwndSliderName = CreateWindowEx(0, L"STATIC", L"Name ", SS_LEFT | WS_CHILD | WS_VISIBLE, 0, 0, 50, 20, hwndGUI, NULL, NULL, NULL);
	hwndSliderValue = CreateWindowEx(0, L"STATIC", L" Value", SS_LEFT | WS_CHILD | WS_VISIBLE, 0, 0, 50, 20, hwndGUI, NULL, NULL, NULL);
	SendMessage(hwndSlider, TBM_SETBUDDY, (WPARAM)TRUE, (LPARAM)hwndSliderName);
	SendMessage(hwndSlider, TBM_SETBUDDY, (WPARAM)FALSE, (LPARAM)hwndSliderValue);

	// Set parameters of the slider
	SendMessage(hwndSlider, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 100));
	SendMessage(hwndSlider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)50);
}

void PointCloudEngine::GUI::ShowContentGeneral()
{
	ShowWindow(hwndDropdown, SW_SHOW);
	ShowWindow(hwndButton, SW_HIDE);
	ShowWindow(hwndSlider, SW_HIDE);
	ShowWindow(hwndSliderName, SW_HIDE);
	ShowWindow(hwndSliderValue, SW_HIDE);
}

void PointCloudEngine::GUI::ShowContentAdvanced()
{
	ShowWindow(hwndDropdown, SW_HIDE);
	ShowWindow(hwndButton, SW_SHOW);
	ShowWindow(hwndSlider, SW_SHOW);
	ShowWindow(hwndSliderName, SW_SHOW);
	ShowWindow(hwndSliderValue, SW_SHOW);
}
