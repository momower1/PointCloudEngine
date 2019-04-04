#ifndef SETTINGS_H
#define SETTINGS_H

#define NAMEOF(variable) #variable

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Settings
    {
    public:
        Settings();
        ~Settings();

        // Rendering parameters default values
        float fovAngleY = 0.4f * XM_PI;
        int resolutionX = 1280;
        int resolutionY = 720;
        int msaaCount = 1;
        bool windowed = true;

        // Ply file parameters default values
        std::wstring plyfile = L"";
        int maxOctreeDepth = 12;
        float scale = 1.0f;

        // Input parameters default values
        float mouseSensitivity = 0.5f;
        float scrollSensitivity = 0.5f;
    };
}

#endif
