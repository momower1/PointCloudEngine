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

        // Rendering parameters default values
        float fovAngleY = 0.4f * XM_PI;
        float nearZ = 0.1f;
        float farZ = 1000.0f;
        int resolutionX = 1280;
        int resolutionY = 720;
        bool windowed = true;

        // Ply file parameters default values
        std::wstring plyfile = L"";
        int maxOctreeDepth = 16;
		float samplingRate = 0.01f;
		float overlapFactor = 2.0f;
		float blendFactor = 2.0f;
		float scale = 1.0f;

		// Lighting parameters
		float lightIntensity = 1.0f;
		float ambient = 0.4f;
		float diffuse = 1.2f;
		float specular = 0.85f;
		float specularExponent = 10.0f;

		// Compute shader default values
		UINT appendBufferCount = 6000000;

        // Input parameters default values
        float mouseSensitivity = 0.005f;
        float scrollSensitivity = 0.5f;
    };
}

#endif
