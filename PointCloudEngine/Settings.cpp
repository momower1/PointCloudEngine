#include "Settings.h"

#define SETTINGS_FILENAME L"/settings.txt"

PointCloudEngine::Settings::Settings()
{
    // Check if the config file exists that stores the last pointcloudFile path
    std::wifstream settingsFile(executableDirectory + SETTINGS_FILENAME);

    if (settingsFile.is_open())
    {
        std::wstring line;

        while (std::getline(settingsFile, line))
        {
            // Split the string
            size_t delimiter = line.find(L"=");

            if (delimiter > 0)
            {
                std::wstring variableName = line.substr(0, delimiter);
                std::wstring variableValue = line.substr(delimiter + 1, line.length());

				// Put both into the map
				settingsMap[variableName] = variableValue;
            }
        }

		// Parse rendering parameters
		TryParse(NAMEOF(backgroundColor), backgroundColor);
		TryParse(NAMEOF(fovAngleY), fovAngleY);
		TryParse(NAMEOF(nearZ), nearZ);
		TryParse(NAMEOF(farZ), farZ);
		TryParse(NAMEOF(resolutionX), resolutionX);
		TryParse(NAMEOF(resolutionY), resolutionY);
		TryParse(NAMEOF(windowed), windowed);
		TryParse(NAMEOF(help), help);
		TryParse(NAMEOF(viewMode), viewMode);

		// Parse pointcloud file parameters
		TryParse(NAMEOF(pointcloudFile), pointcloudFile);
		TryParse(NAMEOF(samplingRate), samplingRate);
		TryParse(NAMEOF(scale), scale);

		// Parse lighting parameters
		TryParse(NAMEOF(useLighting), useLighting);
		TryParse(NAMEOF(useHeadlight), useHeadlight);
		TryParse(NAMEOF(lightDirection), lightDirection);
		TryParse(NAMEOF(lightIntensity), lightIntensity);
		TryParse(NAMEOF(ambient), ambient);
		TryParse(NAMEOF(diffuse), diffuse);
		TryParse(NAMEOF(specular), specular);
		TryParse(NAMEOF(specularExponent), specularExponent);

		// Parse blending parameters
		TryParse(NAMEOF(useBlending), useBlending);
		TryParse(NAMEOF(blendFactor), blendFactor);

		// Parse ground truth parameters
		TryParse(NAMEOF(density), density);
		TryParse(NAMEOF(sparseSamplingRate), sparseSamplingRate);

		// Parse neural network parameters
		TryParse(NAMEOF(useCUDA), useCUDA);
		TryParse(NAMEOF(neuralNetworkScreenArea), neuralNetworkScreenArea);
		TryParse(NAMEOF(neuralNetworkOutputRed), neuralNetworkOutputRed);
		TryParse(NAMEOF(neuralNetworkOutputGreen), neuralNetworkOutputGreen);
		TryParse(NAMEOF(neuralNetworkOutputBlue), neuralNetworkOutputBlue);
		TryParse(NAMEOF(lossCalculationSelf), lossCalculationSelf);
		TryParse(NAMEOF(lossCalculationTarget), lossCalculationTarget);

		// Parse HDF5 dataset generation parameters
		TryParse(NAMEOF(waypointStepSize), waypointStepSize);
		TryParse(NAMEOF(waypointPreviewStepSize), waypointPreviewStepSize);
		TryParse(NAMEOF(sphereStepSize), sphereStepSize);
		TryParse(NAMEOF(sphereMinTheta), sphereMinTheta);
		TryParse(NAMEOF(sphereMaxTheta), sphereMaxTheta);
		TryParse(NAMEOF(sphereMinPhi), sphereMinPhi);
		TryParse(NAMEOF(sphereMaxPhi), sphereMaxPhi);

		// Parse octree parameters
		TryParse(NAMEOF(useOctree), useOctree);
		TryParse(NAMEOF(maxOctreeDepth), maxOctreeDepth);
		TryParse(NAMEOF(overlapFactor), overlapFactor);
		TryParse(NAMEOF(appendBufferCount), appendBufferCount);

		// Parse input parameters
		TryParse(NAMEOF(mouseSensitivity), mouseSensitivity);
		TryParse(NAMEOF(scrollSensitivity), scrollSensitivity);
    }
}

PointCloudEngine::Settings::~Settings()
{
    // Save values as lines with "variableName=variableValue" to file with comments
    std::wofstream settingsFile(executableDirectory + SETTINGS_FILENAME);

	settingsFile << L"# Parameters only apply when restarting the engine!" << std::endl;
	settingsFile << std::endl;

    settingsFile << L"# Rendering Parameters" << std::endl;
	settingsFile << NAMEOF(backgroundColor) << L"=" << ToString(backgroundColor) << std::endl;
    settingsFile << NAMEOF(fovAngleY) << L"=" << fovAngleY << std::endl;
    settingsFile << NAMEOF(nearZ) << L"=" << nearZ << std::endl;
    settingsFile << NAMEOF(farZ) << L"=" << farZ << std::endl;
    settingsFile << NAMEOF(resolutionX) << L"=" << resolutionX << std::endl;
    settingsFile << NAMEOF(resolutionY) << L"=" << resolutionY << std::endl;
    settingsFile << NAMEOF(windowed) << L"=" << windowed << std::endl;
	settingsFile << NAMEOF(help) << L"=" << help << std::endl;
	settingsFile << NAMEOF(viewMode) << L"=" << viewMode << std::endl;
    settingsFile << std::endl;

    settingsFile << L"# Pointcloud File Parameters" << std::endl;
	settingsFile << L"# Delete old .octree files when changing the " << NAMEOF(maxOctreeDepth) << std::endl;
    settingsFile << NAMEOF(pointcloudFile) << L"=" << pointcloudFile << std::endl;
	settingsFile << NAMEOF(samplingRate) << L"=" << samplingRate << std::endl;
    settingsFile << NAMEOF(scale) << L"=" << scale << std::endl;
    settingsFile << std::endl;

	settingsFile << L"# Lighting Parameters" << std::endl;
	settingsFile << NAMEOF(useLighting) << L"=" << useLighting << std::endl;
	settingsFile << NAMEOF(useHeadlight) << L"=" << useHeadlight << std::endl;
	settingsFile << NAMEOF(lightDirection) << L"=" << ToString(lightDirection) << std::endl;
	settingsFile << NAMEOF(lightIntensity) << L"=" << lightIntensity << std::endl;
	settingsFile << NAMEOF(ambient) << L"=" << ambient << std::endl;
	settingsFile << NAMEOF(diffuse) << L"=" << diffuse << std::endl;
	settingsFile << NAMEOF(specular) << L"=" << specular << std::endl;
	settingsFile << NAMEOF(specularExponent) << L"=" << specularExponent << std::endl;
	settingsFile << std::endl;

	settingsFile << L"# Blending Parameters" << std::endl;
	settingsFile << NAMEOF(useBlending) << L"=" << useBlending << std::endl;
	settingsFile << NAMEOF(blendFactor) << L"=" << blendFactor << std::endl;
	settingsFile << std::endl;

	settingsFile << L"# Ground Truth Parameters" << std::endl;
	settingsFile << NAMEOF(density) << L"=" << density << std::endl;
	settingsFile << NAMEOF(sparseSamplingRate) << L"=" << sparseSamplingRate << std::endl;
	settingsFile << std::endl;

	settingsFile << L"# Neural Network Parameters" << std::endl;
	settingsFile << NAMEOF(useCUDA) << L"=" << useCUDA << std::endl;
	settingsFile << NAMEOF(neuralNetworkScreenArea) << L"=" << neuralNetworkScreenArea << std::endl;
	settingsFile << NAMEOF(neuralNetworkOutputRed) << L"=" << neuralNetworkOutputRed << std::endl;
	settingsFile << NAMEOF(neuralNetworkOutputGreen) << L"=" << neuralNetworkOutputGreen << std::endl;
	settingsFile << NAMEOF(neuralNetworkOutputBlue) << L"=" << neuralNetworkOutputBlue << std::endl;
	settingsFile << NAMEOF(lossCalculationSelf) << L"=" << lossCalculationSelf << std::endl;
	settingsFile << NAMEOF(lossCalculationTarget) << L"=" << lossCalculationTarget << std::endl;
	settingsFile << std::endl;

	settingsFile << L"# HDF5 Dataset Generation Parameters" << std::endl;
	settingsFile << NAMEOF(waypointStepSize) << L"=" << waypointStepSize << std::endl;
	settingsFile << NAMEOF(waypointPreviewStepSize) << L"=" << waypointPreviewStepSize << std::endl;
	settingsFile << NAMEOF(sphereStepSize) << L"=" << sphereStepSize << std::endl;
	settingsFile << NAMEOF(sphereMinTheta) << L"=" << sphereMinTheta << std::endl;
	settingsFile << NAMEOF(sphereMaxTheta) << L"=" << sphereMaxTheta << std::endl;
	settingsFile << NAMEOF(sphereMinPhi) << L"=" << sphereMinPhi << std::endl;
	settingsFile << NAMEOF(sphereMaxPhi) << L"=" << sphereMaxPhi << std::endl;
	settingsFile << std::endl;

	settingsFile << L"# Octree Parameters, increase " << NAMEOF(appendBufferCount) << L" when you see flickering" << std::endl;
	settingsFile << NAMEOF(useOctree) << L"=" << useOctree << std::endl;
	settingsFile << NAMEOF(maxOctreeDepth) << L"=" << maxOctreeDepth << std::endl;
	settingsFile << NAMEOF(overlapFactor) << L"=" << overlapFactor << std::endl;
	settingsFile << NAMEOF(appendBufferCount) << L"=" << appendBufferCount << std::endl;
	settingsFile << std::endl;

    settingsFile << L"# Input Parameters" << std::endl;
    settingsFile << NAMEOF(mouseSensitivity) << L"=" << mouseSensitivity << std::endl;
    settingsFile << NAMEOF(scrollSensitivity) << L"=" << scrollSensitivity << std::endl;
    settingsFile << std::endl;

    settingsFile.flush();
    settingsFile.close();
}

std::wstring PointCloudEngine::Settings::ToString(Vector3 v)
{
	return L"(" + std::to_wstring(v.x) + L"," + std::to_wstring(v.y) + L"," + std::to_wstring(v.z) + L")";
}

std::wstring PointCloudEngine::Settings::ToString(Vector4 v)
{
	return L"(" + std::to_wstring(v.x) + L"," + std::to_wstring(v.y) + L"," + std::to_wstring(v.z) + L"," + std::to_wstring(v.w) + L")";
}

Vector3 PointCloudEngine::Settings::ToVector3(std::wstring s)
{
	// Remove brackets
	s = s.substr(1, s.length() - 1);

	// Parse the string into seperate x,y,z strings
	std::wstring x = s.substr(0, s.find(L','));
	s = s.substr(x.length() + 1, s.length());
	std::wstring y = s.substr(0, s.find(L','));
	std::wstring z = s.substr(y.length() + 1, s.length());
	
	return Vector3(std::stof(x), std::stof(y), std::stof(z));
}

Vector4 PointCloudEngine::Settings::ToVector4(std::wstring s)
{
	// Remove brackets
	s = s.substr(1, s.length() - 1);

	// Parse the string into seperate x,y,z,w strings
	std::wstring x = s.substr(0, s.find(L','));
	s = s.substr(x.length() + 1, s.length());
	std::wstring y = s.substr(0, s.find(L','));
	s = s.substr(y.length() + 1, s.length());
	std::wstring z = s.substr(0, s.find(L','));
	std::wstring w = s.substr(z.length() + 1, s.length());

	return Vector4(std::stof(x), std::stof(y), std::stof(z), std::stof(w));
}

void PointCloudEngine::Settings::TryParse(std::wstring parameterName, float& outParameterValue)
{
	if (settingsMap.find(parameterName) != settingsMap.end())
	{
		outParameterValue = std::stof(settingsMap[parameterName]);
	}
}

void PointCloudEngine::Settings::TryParse(std::wstring parameterName, int& outParameterValue)
{
	if (settingsMap.find(parameterName) != settingsMap.end())
	{
		outParameterValue = std::stoi(settingsMap[parameterName]);
	}
}

void PointCloudEngine::Settings::TryParse(std::wstring parameterName, UINT& outParameterValue)
{
	if (settingsMap.find(parameterName) != settingsMap.end())
	{
		outParameterValue = std::stoi(settingsMap[parameterName]);
	}
}

void PointCloudEngine::Settings::TryParse(std::wstring parameterName, bool& outParameterValue)
{
	if (settingsMap.find(parameterName) != settingsMap.end())
	{
		outParameterValue = std::stoi(settingsMap[parameterName]);
	}
}

void PointCloudEngine::Settings::TryParse(std::wstring parameterName, Vector3& outParameterValue)
{
	if (settingsMap.find(parameterName) != settingsMap.end())
	{
		outParameterValue = ToVector3(settingsMap[parameterName]);
	}
}

void PointCloudEngine::Settings::TryParse(std::wstring parameterName, Vector4& outParameterValue)
{
	if (settingsMap.find(parameterName) != settingsMap.end())
	{
		outParameterValue = ToVector4(settingsMap[parameterName]);
	}
}

void PointCloudEngine::Settings::TryParse(std::wstring parameterName, std::wstring& outParameterValue)
{
	if (settingsMap.find(parameterName) != settingsMap.end())
	{
		outParameterValue = settingsMap[parameterName];
	}
}
