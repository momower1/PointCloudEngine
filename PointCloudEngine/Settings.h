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
		bool help = false;
		int viewMode = 0;

        // Pointcloud file parameters default values
        std::wstring pointcloudFile = L"";
		float samplingRate = 0.01f;
		float scale = 1.0f;

		// Lighting parameters
		bool useLighting = true;
		float lightIntensity = 1.0f;
		float ambient = 0.4f;
		float diffuse = 1.2f;
		float specular = 0.85f;
		float specularExponent = 10.0f;

		// Blending parameters
		bool useBlending = true;
		float blendFactor = 2.0f;

		// Ground truth parameters
		float density = 1.0f;
		float sparseSamplingRate = 0.01f;

		// HDF5 dataset generation parameters
		float stepSize = 0.5f;
		float minTheta = 0;
		float maxTheta = XM_PI;
		float minPhi = 0;
		float maxPhi = 2 * XM_PI;

		// Octree parameters
		bool useOctree = false;
		int maxOctreeDepth = 16;
		float overlapFactor = 2.0f;
		UINT appendBufferCount = 6000000;

        // Input parameters default values
        float mouseSensitivity = 0.005f;
        float scrollSensitivity = 0.5f;
    };
}

#endif
