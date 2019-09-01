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

                // Parse by variable name
                if (variableName.compare(NAMEOF(fovAngleY)) == 0)
                {
                    fovAngleY = std::stof(variableValue);
                }
                else if (variableName.compare(NAMEOF(nearZ)) == 0)
                {
                    nearZ = std::stof(variableValue);
                }
                else if (variableName.compare(NAMEOF(farZ)) == 0)
                {
                    farZ = std::stof(variableValue);
                }
                else if (variableName.compare(NAMEOF(resolutionX)) == 0)
                {
                    resolutionX = std::stoi(variableValue);
                }
                else if (variableName.compare(NAMEOF(resolutionY)) == 0)
                {
                    resolutionY = std::stoi(variableValue);
                }
                else if (variableName.compare(NAMEOF(windowed)) == 0)
                {
                    windowed = std::stoi(variableValue);
                }
				else if (variableName.compare(NAMEOF(help)) == 0)
				{
					help = std::stoi(variableValue);
				}
				else if (variableName.compare(NAMEOF(viewMode)) == 0)
				{
					viewMode = std::stoi(variableValue);
				}
                else if (variableName.compare(NAMEOF(pointcloudFile)) == 0)
                {
                    pointcloudFile = variableValue;
                }
				else if (variableName.compare(NAMEOF(samplingRate)) == 0)
				{
					samplingRate = std::stof(variableValue);
				}
                else if (variableName.compare(NAMEOF(scale)) == 0)
                {
                    scale = std::stof(variableValue);
                }
				else if (variableName.compare(NAMEOF(useLighting)) == 0)
				{
					useLighting = std::stoi(variableValue);
				}
				else if (variableName.compare(NAMEOF(lightDirection)) == 0)
				{
					lightDirection = ToVector3(variableValue);
				}
				else if (variableName.compare(NAMEOF(lightIntensity)) == 0)
				{
					lightIntensity = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(ambient)) == 0)
				{
					ambient = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(diffuse)) == 0)
				{
					diffuse = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(specular)) == 0)
				{
					specular = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(specularExponent)) == 0)
				{
					specularExponent = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(useBlending)) == 0)
				{
					useBlending = std::stoi(variableValue);
				}
				else if (variableName.compare(NAMEOF(blendFactor)) == 0)
				{
					blendFactor = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(density)) == 0)
				{
					density = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(sparseSamplingRate)) == 0)
				{
					sparseSamplingRate = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(stepSize)) == 0)
				{
					stepSize = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(minTheta)) == 0)
				{
					minTheta = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(maxTheta)) == 0)
				{
					maxTheta = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(minPhi)) == 0)
				{
					minPhi = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(maxPhi)) == 0)
				{
					maxPhi = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(useOctree)) == 0)
				{
					useOctree = std::stoi(variableValue);
				}
				else if (variableName.compare(NAMEOF(maxOctreeDepth)) == 0)
				{
					maxOctreeDepth = std::stoi(variableValue);
				}
				else if (variableName.compare(NAMEOF(overlapFactor)) == 0)
				{
					overlapFactor = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(appendBufferCount)) == 0)
				{
					appendBufferCount = std::stoi(variableValue);
				}
                else if (variableName.compare(NAMEOF(mouseSensitivity)) == 0)
                {
                    mouseSensitivity = std::stof(variableValue);
                }
                else if (variableName.compare(NAMEOF(scrollSensitivity)) == 0)
                {
                    scrollSensitivity = std::stof(variableValue);
                }
            }
        }
    }
}

PointCloudEngine::Settings::~Settings()
{
    // Save values as lines with "variableName=variableValue" to file with comments
    std::wofstream settingsFile(executableDirectory + SETTINGS_FILENAME);

	settingsFile << L"# Parameters only apply when restarting the engine!" << std::endl;
	settingsFile << std::endl;

    settingsFile << L"# Rendering Parameters" << std::endl;
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

	settingsFile << L"# HDF5 Dataset Generation Parameters" << std::endl;
	settingsFile << NAMEOF(stepSize) << L"=" << stepSize << std::endl;
	settingsFile << NAMEOF(minTheta) << L"=" << minTheta << std::endl;
	settingsFile << NAMEOF(maxTheta) << L"=" << maxTheta << std::endl;
	settingsFile << NAMEOF(minPhi) << L"=" << minPhi << std::endl;
	settingsFile << NAMEOF(maxPhi) << L"=" << maxPhi << std::endl;
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
