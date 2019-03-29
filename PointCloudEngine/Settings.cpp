#include "Settings.h"

#define SETTINGS_FILENAME L"/settings.pointcloudengine"

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
            int delimiter = line.find(L"=");

            if (delimiter > 0)
            {
                std::wstring variableNameW = line.substr(0, delimiter);
                std::wstring variableValue = line.substr(delimiter + 1, line.length());

                // nameof doesn't work with wstring sadly, convert to string
                std::string variableName = std::string(variableNameW.begin(), variableNameW.end());

                // Parse by variable name
                if (variableName.compare(nameof(fovAngleY)) == 0)
                {
                    fovAngleY = std::stof(variableValue);
                }

                if (variableName.compare(nameof(resolutionX)) == 0)
                {
                    resolutionX = std::stoi(variableValue);
                }

                if (variableName.compare(nameof(resolutionY)) == 0)
                {
                    resolutionY = std::stoi(variableValue);
                }

                if (variableName.compare(nameof(msaaCount)) == 0)
                {
                    msaaCount = std::stoi(variableValue);
                }

                if (variableName.compare(nameof(windowed)) == 0)
                {
                    windowed = std::stoi(variableValue);
                }

                if (variableName.compare(nameof(plyfile)) == 0)
                {
                    plyfile = variableValue;
                }

                if (variableName.compare(nameof(maxOctreeDepth)) == 0)
                {
                    maxOctreeDepth = std::stoi(variableValue);
                }

                if (variableName.compare(nameof(scale)) == 0)
                {
                    scale = std::stof(variableValue);
                }

                if (variableName.compare(nameof(mouseSensitivity)) == 0)
                {
                    mouseSensitivity = std::stof(variableValue);
                }

                if (variableName.compare(nameof(scrollSensitivity)) == 0)
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

    settingsFile << L"# In order to change parameters:" << std::endl;
    settingsFile << L"# Close the engine, edit this file and then restard the engine" << std::endl;

    settingsFile << L"# Rendering Parameters" << std::endl;
    settingsFile << nameof(fovAngleY) << L"=" << fovAngleY << std::endl;
    settingsFile << nameof(resolutionX) << L"=" << resolutionX << std::endl;
    settingsFile << nameof(resolutionY) << L"=" << resolutionY << std::endl;
    settingsFile << nameof(msaaCount) << L"=" << msaaCount << std::endl;
    settingsFile << nameof(windowed) << L"=" << windowed << std::endl;
    settingsFile << std::endl;

    settingsFile << L"# Ply File Parameters" << std::endl;
    settingsFile << nameof(plyfile) << L"=" << plyfile << std::endl;
    settingsFile << nameof(maxOctreeDepth) << L"=" << maxOctreeDepth << std::endl;
    settingsFile << nameof(scale) << L"=" << scale << std::endl;
    settingsFile << std::endl;

    settingsFile << L"# Input Parameters" << std::endl;
    settingsFile << nameof(mouseSensitivity) << L"=" << mouseSensitivity << std::endl;
    settingsFile << nameof(scrollSensitivity) << L"=" << scrollSensitivity << std::endl;
    settingsFile << std::endl;

    settingsFile.flush();
    settingsFile.close();
}
