#include "Utils.h"

Gdiplus::RectF Utils::GetGdiplusRect(RECT rect)
{
    Gdiplus::RectF gdiplusRect;
    gdiplusRect.X = rect.left;
    gdiplusRect.Y = rect.top;
    gdiplusRect.Width = rect.right - rect.left;
    gdiplusRect.Height = rect.bottom - rect.top;

    return gdiplusRect;
}

std::vector<BYTE> Utils::LoadResourceBinary(DWORD resourceID, std::wstring resourceType)
{
    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(resourceID), resourceType.c_str());
    ERROR_MESSAGE_ON_NULL(hResource, NAMEOF(FindResource) + L" failed!");

    HGLOBAL resourceBuffer = LoadResource(NULL, hResource);
    ERROR_MESSAGE_ON_NULL(resourceBuffer, NAMEOF(LoadResource) + L" failed!");

    const BYTE* resourceData = (const BYTE*)LockResource(resourceBuffer);
    const DWORD resourceDataSize = SizeofResource(NULL, hResource);

    std::vector<BYTE> resourceBinary(resourceDataSize);
    memcpy(resourceBinary.data(), resourceData, resourceDataSize);

    FreeResource(resourceBuffer);

    return resourceBinary;
}

Gdiplus::Image* Utils::LoadImageFromResource(DWORD resourceID, std::wstring resourceType)
{
    std::vector<BYTE> resourceBytes = LoadResourceBinary(resourceID, resourceType);
    IStream* stream = SHCreateMemStream(resourceBytes.data(), resourceBytes.size());
    Gdiplus::Image* image = Gdiplus::Image::FromStream(stream);
    stream->Release();

    return image;
}