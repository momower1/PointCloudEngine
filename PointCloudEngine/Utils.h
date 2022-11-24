#ifndef UTILS_H
#define UTILS_H

#include "PointCloudEngine.h"

#define SAFE_RELEASE(pointer) if (pointer != NULL) { pointer->Release(); pointer = NULL; }
#define SAFE_DELETE(pointer) if (pointer != NULL) { delete pointer; pointer = NULL; }
#define SAFE_CLOSE(winrtObject) if (winrtObject) { winrtObject.Close(); }
#define NAMEOF(anything) std::wstring(L#anything)
#define INFO_MESSAGE(message) MessageBox(NULL, std::wstring(message).c_str(), L"Information", MB_ICONINFORMATION | MB_TOPMOST | MB_OK);
#define INFO_MESSAGE_ON_NULL(pointer, message) if (pointer == NULL) { INFO_MESSAGE(message); }
#define INFO_MESSAGE_ON_FAIL(success, message) if (!success) { INFO_MESSAGE(message); }
#define INFO_MESSAGE_ON_HR(hr, message) if (FAILED(hr)) { INFO_MESSAGE(message); }
#define WARNING_MESSAGE(message) MessageBox(NULL, std::wstring(message).c_str(), L"Warning", MB_ICONWARNING | MB_TOPMOST | MB_OK);
#define WARNING_MESSAGE_ON_NULL(pointer, message) if (pointer == NULL) { WARNING_MESSAGE(message); }
#define WARNING_MESSAGE_ON_FAIL(success, message) if (!success) { WARNING_MESSAGE(message); }
#define WARNING_MESSAGE_ON_HR(hr, message) if (FAILED(hr)) { WARNING_MESSAGE(message); }
#define ERROR_MESSAGE(message) MessageBox(NULL, std::wstring(message).c_str(), L"Error", MB_ICONERROR | MB_TOPMOST | MB_OK); exit(EXIT_FAILURE);
#define ERROR_MESSAGE_ON_NULL(pointer, message) if (pointer == NULL) { ERROR_MESSAGE(message); }
#define ERROR_MESSAGE_ON_FAIL(success, message) if (!success) { ERROR_MESSAGE(message); }
#define ERROR_MESSAGE_ON_HR(hr, message) if (FAILED(hr)) { ERROR_MESSAGE(message); }
#define CONTINUE_ON_FAIL(success, message) if (!success) { continue; }
#define CONTINUE_ON_HR(hr, message) if (FAILED(hr)) { continue; }
#define RETURN_ON_FAIL(success, message) if (!success) { return; }
#define RETURN_ON_HR(hr, message) if (FAILED(hr)) { return; }
#define GUI_SCALED(number) (settings->guiScaleFactor * number)
#define GS(number) GUI_SCALED(number)

class Utils
{
public:
	static Gdiplus::RectF GetGdiplusRect(RECT rect);
	static std::vector<BYTE> LoadResourceBinary(DWORD resourceID, std::wstring resourceType);
	static Gdiplus::Image* LoadImageFromResource(DWORD resourceID, std::wstring resourceType);
	static std::vector<std::wstring> SplitString(std::wstring string, std::wstring splitter);
};

#endif