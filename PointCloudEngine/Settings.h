#ifndef SETTINGS_H
#define SETTINGS_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Settings
    {
    public:
        Settings();
        ~Settings();

        // Rendering parameters
        float fovAngleY = 0.4f * XM_PI;
        int resolutionX = 1280;
        int resolutionY = 720;
        int msaaCount = 1;
        bool windowed = true;

        // Ply file parameters
        std::wstring plyfile = L"";
        int maxOctreeDepth = 12;
        float scale = 1.0f;

        // Input parameters
        float mouseSensitivity = 1.0f;
        float scrollSensitivity = 1.0f;
    };
}

#endif
