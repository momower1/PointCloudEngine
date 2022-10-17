#ifndef SETTINGS_H
#define SETTINGS_H

#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Settings
    {
    public:
        Settings(std::wstring filename);
        ~Settings();
		std::wstring ToKeyValueString();
		std::wstring ToString(Vector3 v);
		std::wstring ToString(Vector4 v);
		Vector3 ToVector3(std::wstring s);
		Vector4 ToVector4(std::wstring s);

		// Constants that cannot be edited
		const int userInterfaceWidth = 380;
		const int userInterfaceHeight = 540;

		// Engine window parameters
		float guiScaleFactor = GetDpiForSystem() / 96.0f;
		int engineWidth = 1280;
		int engineHeight = 720;
		int enginePositionX = GetSystemMetrics(SM_CXSCREEN) / 2;
		int enginePositionY = GetSystemMetrics(SM_CYSCREEN) / 2;
		bool showUserInterface = true;

        // Rendering parameters default values
		ViewMode viewMode = ViewMode::Points;
		Vector4 backgroundColor = Vector4(0, 0, 0, 0);
        float fovAngleY = 0.4f * XM_PI;
        float nearZ = 0.1f;
        float farZ = 1000.0f;
        int resolutionX = 512;
        int resolutionY = 512;
        bool windowed = true;

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
		bool backfaceCulling = true;
		float density = 0.2f;
		float sparseSamplingRate = 0.01f;

		// Pull push parameters
		bool pullPushLinearFilter = true;
		bool pullPushSkipPushPhase = false;
		int pullPushDebugLevel = 0;
		float pullPushSplatSize = 0.25f;

		// Neural Network parameters
		std::wstring neuralNetworkModelFile = L"";
		std::wstring neuralNetworkDescriptionFile = L"";
		bool useCUDA = true;
		float neuralNetworkLossArea = 0.5f;
		int neuralNetworkOutputRed = 0;
		int neuralNetworkOutputGreen = 1;
		int neuralNetworkOutputBlue = 2;
		std::wstring lossCalculationSelf = L"SplatsColor";
		std::wstring lossCalculationTarget = L"SplatsColor";

		// HDF5 dataset generation parameters
		int downsampleFactor = 2;
		float waypointStepSize = 0.5f;
		float waypointPreviewStepSize = 0.1f;
		float waypointMin = 0.0f;
		float waypointMax = 1.0f;
		float sphereStepSize = 0.5f;
		float sphereMinTheta = 0;
		float sphereMaxTheta = XM_PI;
		float sphereMinPhi = 0;
		float sphereMaxPhi = 2 * XM_PI;

		// Octree parameters
		bool useOctree = false;
		bool useCulling = true;
		bool useGPUTraversal = true;
		int octreeLevel = -1;
		int maxOctreeDepth = 16;
		float overlapFactor = 2.0f;
		float splatResolution = 0.01f;
		UINT appendBufferCount = 6000000;

        // Input parameters default values
        float mouseSensitivity = 0.005f;
        float scrollSensitivity = 0.5f;

	private:
		std::wstring filename;
		std::map<std::wstring, std::wstring> settingsMap;

		template<typename T> void TryParse(std::wstring parameterName, T* outParameterValue)
		{
			if (settingsMap.find(parameterName) != settingsMap.end())
			{
				if (typeid(T) == typeid(std::wstring))
				{
					*((std::wstring*)outParameterValue) = settingsMap[parameterName];
				}
				else if (typeid(T) == typeid(float))
				{
					*((float*)outParameterValue) = std::stof(settingsMap[parameterName]);
				}
				else if (typeid(T) == typeid(int))
				{
					*((int*)outParameterValue) = std::stoi(settingsMap[parameterName]);
				}
				else if (typeid(T) == typeid(UINT))
				{
					*((UINT*)outParameterValue) = std::stoi(settingsMap[parameterName]);
				}
				else if (typeid(T) == typeid(bool))
				{
					*((bool*)outParameterValue) = std::stoi(settingsMap[parameterName]);
				}
				else if (typeid(T) == typeid(Vector3))
				{
					*((Vector3*)outParameterValue) = ToVector3(settingsMap[parameterName]);
				}
				else if (typeid(T) == typeid(Vector4))
				{
					*((Vector4*)outParameterValue) = ToVector4(settingsMap[parameterName]);
				}
				else if (typeid(T) == typeid(ViewMode))
				{
					*((ViewMode*)outParameterValue) = (ViewMode)std::stoi(settingsMap[parameterName]);
				}
				else
				{
					ERROR_MESSAGE(NAMEOF(TryParse) + L" cannot parse " + parameterName + L" because its type is unknown!");
				}
			}
		};
    };
}

#endif