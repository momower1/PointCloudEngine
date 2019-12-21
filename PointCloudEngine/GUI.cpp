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

	// Load a specific font for DrawText and TextOut functions
	HFONT segoe = CreateFont(24, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, TEXT("Segoe UI"));
	hdcGeneral = GetDC(hwndGeneral);
	SelectObject(hdcGeneral, segoe);

	hdcAdvanced = GetDC(hwndAdvanced);
	SelectObject(hdcAdvanced, segoe);
}

PointCloudEngine::GUI::~GUI()
{
	ReleaseDC(hwndGeneral, hdcGeneral);
	ReleaseDC(hwndAdvanced, hdcAdvanced);
}

void PointCloudEngine::GUI::Update()
{
	// Text on general tab
	TextOut(hdcGeneral, 10, 200, L"Sample with font!", 17);

	// Text on advanced tab
	RECT rect;
	GetClientRect(hwndAdvanced, &rect);
	DrawText(hdcAdvanced, L"Hello World!", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void PointCloudEngine::GUI::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Tab handler
	if ((msg == WM_NOTIFY) && ((LPNMHDR)lParam)->code == TCN_SELCHANGE)
	{
		if (TabCtrl_GetCurSel(hwndTab) == 0)
		{
			ShowWindow(hwndGeneral, SW_SHOW);
			ShowWindow(hwndAdvanced, SW_HIDE);
		}
		else
		{
			ShowWindow(hwndGeneral, SW_HIDE);
			ShowWindow(hwndAdvanced, SW_SHOW);
		}
	}


	OutputDebugString((std::to_wstring(msg) + L" " + std::to_wstring(wParam) + L" " + std::to_wstring(lParam)).c_str());
}

void PointCloudEngine::GUI::CreateContentGeneral()
{
	// For the general tab
	hwndGeneral = CreateWindowEx(NULL, L"STATIC", L"", WS_VISIBLE | WS_CHILD, 0, 25, guiSize.x, guiSize.y, hwndTab, NULL, NULL, NULL);
	hwndDropdown = CreateWindowEx(NULL, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE, 25, 50, 100, 60, hwndGeneral, NULL, NULL, NULL);

	// Add items to the dropdown
	SendMessage(hwndDropdown, CB_ADDSTRING, 0, (LPARAM)L"Eagle");
	SendMessage(hwndDropdown, CB_ADDSTRING, 0, (LPARAM)L"Hamster");
	SendMessage(hwndDropdown, CB_SETCURSEL, 0, 0);
}

void PointCloudEngine::GUI::CreateContentAdvanced()
{
	// For the advanced tab
	hwndAdvanced = CreateWindowEx(NULL, L"STATIC", L"", WS_CHILD, 0, 25, guiSize.x, guiSize.y, hwndTab, NULL, NULL, NULL);
	hwndSlider = CreateWindowEx(NULL, TRACKBAR_CLASS, L"", WS_CHILD | WS_VISIBLE, 25, 100, 100, 20, hwndAdvanced, NULL, NULL, NULL);

	// Set parameters of the slider
	SendMessage(hwndSlider, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 100));
	SendMessage(hwndSlider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)50);
}
