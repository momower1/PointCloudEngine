#include "Settings.h"

#define SETTINGS_FILENAME L"/settings.txt"

PointCloudEngine::Settings::Settings()
{
    // Check if the config file exists that stores the last plyfile path
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
                else if (variableName.compare(NAMEOF(plyfile)) == 0)
                {
                    plyfile = variableValue;
                }
                else if (variableName.compare(NAMEOF(maxOctreeDepth)) == 0)
                {
                    maxOctreeDepth = std::stoi(variableValue);
                }
				else if (variableName.compare(NAMEOF(samplingRate)) == 0)
				{
					samplingRate = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(overlapFactor)) == 0)
				{
					overlapFactor = std::stof(variableValue);
				}
				else if (variableName.compare(NAMEOF(depthEpsilon)) == 0)
				{
					depthEpsilon = std::stof(variableValue);
				}
                else if (variableName.compare(NAMEOF(scale)) == 0)
                {
                    scale = std::stof(variableValue);
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
    settingsFile << std::endl;

    settingsFile << L"# Ply File Parameters" << std::endl;
	settingsFile << L"# Delete old .octree files when changing the " << NAMEOF(maxOctreeDepth) << std::endl;
    settingsFile << NAMEOF(plyfile) << L"=" << plyfile << std::endl;
    settingsFile << NAMEOF(maxOctreeDepth) << L"=" << maxOctreeDepth << std::endl;
	settingsFile << NAMEOF(samplingRate) << L"=" << samplingRate << std::endl;
	settingsFile << NAMEOF(overlapFactor) << L"=" << overlapFactor << std::endl;
	settingsFile << NAMEOF(depthEpsilon) << L"=" << depthEpsilon << std::endl;
    settingsFile << NAMEOF(scale) << L"=" << scale << std::endl;
    settingsFile << std::endl;

	settingsFile << L"# Lighting Parameters" << std::endl;
	settingsFile << NAMEOF(lightIntensity) << L"=" << lightIntensity << std::endl;
	settingsFile << NAMEOF(ambient) << L"=" << ambient << std::endl;
	settingsFile << NAMEOF(diffuse) << L"=" << diffuse << std::endl;
	settingsFile << NAMEOF(specular) << L"=" << specular << std::endl;
	settingsFile << NAMEOF(specularExponent) << L"=" << specularExponent << std::endl;
	settingsFile << std::endl;

	settingsFile << L"# Compute Shader Parameters" << std::endl;
	settingsFile << L"# Increase " << NAMEOF(appendBufferCount) << L" when you see flickering" << std::endl;
	settingsFile << NAMEOF(appendBufferCount) << L"=" << appendBufferCount << std::endl;
	settingsFile << std::endl;

    settingsFile << L"# Input Parameters" << std::endl;
    settingsFile << NAMEOF(mouseSensitivity) << L"=" << mouseSensitivity << std::endl;
    settingsFile << NAMEOF(scrollSensitivity) << L"=" << scrollSensitivity << std::endl;
    settingsFile << std::endl;

    settingsFile.flush();
    settingsFile.close();
}
