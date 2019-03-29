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

        float fovAngleY = 0.4f * XM_PI;
        int resolutionX = 1280;
        int resolutionY = 720;
        int msaaCount = 1;
        bool windowed = true;
        std::wstring plyfile;
        float scale = 1.0f;
    };
}

#endif
