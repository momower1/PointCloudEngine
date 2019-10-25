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
		Vector4 backgroundColor = Vector4(0, 0, 0, 0);
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
		bool useHeadlight = true;
		Vector3 lightDirection = Vector3(0, -0.5f, 1.0f);
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

		// Neural Network parameters
		bool useCUDA = true;
		float neuralNetworkScreenArea = 0.5f;
		int neuralNetworkOutputRed = 0;
		int neuralNetworkOutputGreen = 1;
		int neuralNetworkOutputBlue = 2;

		// HDF5 dataset generation parameters
		float waypointStepSize = 0.5f;
		float waypointPreviewStepSize = 0.1f;
		float sphereStepSize = 0.5f;
		float sphereMinTheta = 0;
		float sphereMaxTheta = XM_PI;
		float sphereMinPhi = 0;
		float sphereMaxPhi = 2 * XM_PI;

		// Octree parameters
		bool useOctree = false;
		int maxOctreeDepth = 16;
		float overlapFactor = 2.0f;
		UINT appendBufferCount = 6000000;

        // Input parameters default values
        float mouseSensitivity = 0.005f;
        float scrollSensitivity = 0.5f;

		std::wstring ToString(Vector3 v);
		std::wstring ToString(Vector4 v);
		Vector3 ToVector3(std::wstring s);
		Vector4 ToVector4(std::wstring s);

	private:
		std::map<std::wstring, std::wstring> settingsMap;

		void TryParse(std::wstring parameterName, float& outParameterValue);
		void TryParse(std::wstring parameterName, int& outParameterValue);
		void TryParse(std::wstring parameterName, UINT& outParameterValue);
		void TryParse(std::wstring parameterName, bool& outParameterValue);
		void TryParse(std::wstring parameterName, Vector3& outParameterValue);
		void TryParse(std::wstring parameterName, Vector4& outParameterValue);
		void TryParse(std::wstring parameterName, std::wstring& outParameterValue);
    };
}

#endif
